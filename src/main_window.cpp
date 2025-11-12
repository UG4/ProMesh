/*
 * Copyright (c) 2008-2015:  G-CSC, Goethe University Frankfurt
 * Copyright (c) 2006-2008:  Steinbeis Forschungszentrum (STZ Ölbronn)
 * Copyright (c) 2006-2015:  Sebastian Reiter
 * Author: Sebastian Reiter
 *
 * This file is part of ProMesh.
 * 
 * ProMesh is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 3 (as published by the
 * Free Software Foundation) with the following additional attribution
 * requirements (according to LGPL/GPL v3 §7):
 * 
 * (1) The following notice must be displayed in the Appropriate Legal Notices
 * of covered and combined works: "Based on ProMesh (www.promesh3d.com)".
 * 
 * (2) The following bibliography is recommended for citation and must be
 * preserved in all covered files:
 * "Reiter, S. and Wittum, G. ProMesh -- a flexible interactive meshing software
 *   for unstructured hybrid grids in 1, 2, and 3 dimensions. In preparation."
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 */

#include <iostream>
#include <QtWidgets>
#include <QDesktopServices>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include "main_window.hpp"
#include "view3d/view3d.hpp"
#include "scene/lg_scene.hpp"
#include "scene/csg_object.hpp"
#include "scene_inspector.hpp"
#include "scene_item_model.hpp"
#include "QDebugStream.hpp"
#include "lib_grid/lib_grid.h"
#include "lib_grid/file_io/file_io_ug.h"
#include "lib_grid/file_io/file_io_ugx.h"
#include "lib_grid/file_io/file_io_lgb.h"
#include "undo.hpp"
#include "app.hpp"
#include "widgets/coordinates_widget.hpp"
#include "common/util/file_util.h"
#include "bridge/bridge.h"
#include "common/util/path_provider.h"
#include "common/util/plugin_util.h"
#include "options/options.hpp"
#include "util/file_util.hpp"
#include "modules/mesh_module.hpp"
#include "widgets/property_widget.hpp"
#include "widgets/truncated_double_spin_box.hpp"
#include "widgets/widget_list.hpp"

#ifdef PROMESH_USE_WEBKIT
	#include "widgets/help_browser.h"
#endif

//tmp
#include <QDir>
#include <QFile>
#include <QFileInfo>

using namespace std;
using namespace ug;

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//	implementation of MainWindow

////////////////////////////////////////////////////////////////////////
//	constructor
MainWindow::MainWindow() :
	_active_module (nullptr),
	_settings(),
	_selection_element(0),
	_selection_mode(0),
	_cur_selection_mode(-1),
	_element_mode_list_index(3),
	_mouse_move_action(MMA_DEFAULT),
	_active_axis(X_AXIS | Y_AXIS | Z_AXIS),
	_active_object(nullptr),
	_action_log_sender(nullptr),
	#ifdef PROMESH_USE_WEBKIT
		m_helpBrowser(nullptr),
	#endif
	_dlg_about(nullptr)
{
}

void MainWindow::init()
{ 
	setObjectName(tr("main_window"));
	setAcceptDrops(true);
	// setGeometry(0, 0, 1024, 600);


//	create view and scene
	_p_view = new View3D;
	setCentralWidget(_p_view);
	connect(_p_view, &View3D::mousePressed, this, &MainWindow::view3dMousePressed);
	connect(_p_view, &View3D::mouseMoved, this, &MainWindow::view3dMouseMoved);
	connect(_p_view, &View3D::mouseReleased, this, &MainWindow::view3dMouseReleased);
	connect(_p_view, &View3D::keyReleased, this, &MainWindow::view3dKeyReleased);

	_scene = new LGScene;

	_p_view->set_renderer(_scene);
	connect(_scene, &IScene::visuals_updated, _p_view, static_cast<void (View3D::*)()>(&View3D::update));


	setTabPosition(Qt::BottomDockWidgetArea, QTabWidget::West);
//	create the log widget
	_p_log = new QDockWidget(tr("log"), this);
	_p_log->setFeatures(QDockWidget::NoDockWidgetFeatures);
	_p_log->setObjectName(tr("log"));

	QFont logFont("unknown");
	logFont.setStyleHint(QFont::Monospace);
	logFont.setPointSize(10);
	_p_log_text = new QPlainTextEdit(_p_log);
	_p_log_text->setReadOnly(true);
	_p_log_text->setUndoRedoEnabled(false);
	_p_log_text->setWordWrapMode(QTextOption::NoWrap);
	_p_log_text->setFont(logFont);
	_p_log->setWidget(_p_log_text);

	addDockWidget(Qt::BottomDockWidgetArea, _p_log);

//	action log dock
	auto actionLogDock = new QDockWidget(tr("actions"), this);
	actionLogDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
	actionLogDock->setObjectName(tr("actionLog"));

	_action_log = new QPlainTextEdit(actionLogDock);
	_action_log->setReadOnly(true);
	_action_log->setUndoRedoEnabled(false);
	_action_log->setWordWrapMode(QTextOption::NoWrap);
	_action_log->setFont(logFont);
	actionLogDock->setWidget(_action_log);
	addDockWidget(Qt::BottomDockWidgetArea, actionLogDock);
	tabifyDockWidget(_p_log, actionLogDock);

//	option dock
	auto* optionDock = new QDockWidget(tr("options"), this);
	optionDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
	optionDock->setObjectName(tr("options"));
	_opt_widget = new PropertyWidget(optionDock);
	connect(_opt_widget, &PropertyWidget::valueChanged, this, &MainWindow::optionsChanged);
	optionDock->setWidget(_opt_widget);
	loadOptions();
	tabifyDockWidget(actionLogDock, optionDock);

//	redirect cout
	auto* pDebugStream = new Q_DebugStream(GetLogAssistant().logger(), _p_log_text);
	pDebugStream->enable_file_output(app::UserDataDir().path() + QString("/log.txt"));

	UG_LOG(GetFileContent(":/resources/greetings.txt").toStdString() << endl);


	try{
		bridge::InitBridge();
		if(!LoadPlugins(PathProvider::get_path(PLUGIN_PATH).c_str(), "ug4/", bridge::GetUGRegistry()))
		{
			UG_LOG("ERROR during initialization of plugins: LoadPlugins failed!\n");
		}
	}
	catch(UGError& err){
		UG_LOG("ERROR during initialization of ug::bridge:\n");
		for(size_t i = 0; i < err.num_msg(); ++i){
			UG_LOG("  " << err.get_msg(i) << std::endl);
		}
	}


//	file-menu
	_action_new = new QAction(tr("&New"), this);
	_action_new->setIcon(QIcon(":images/filenew.png"));
	_action_new->setShortcut(tr("Ctrl+N"));
	_action_new->setToolTip(tr("Create a new empty geometry-object."));
	connect(_action_new, &QAction::triggered, this, &MainWindow::newGeometry);

	_action_open = new QAction(tr("&Open"), this);
	_action_open->setIcon(QIcon(":images/fileopen.png"));
	_action_open->setShortcut(tr("Ctrl+O"));
	_action_open->setToolTip(tr("Load a geometry from file."));
	connect(_action_open, &QAction::triggered, this, &MainWindow::openFile);

	_action_loac_into_mesh = new QAction(tr("&Load Into Mesh"), this);
	_action_loac_into_mesh->setIcon(QIcon(":images/fileopen.png"));
	_action_loac_into_mesh->setShortcut(tr("Ctrl+L"));
	_action_loac_into_mesh->setToolTip(tr("Load geometries from file and add it to the current mesh."));
	connect(_action_loac_into_mesh, &QAction::triggered, this, &MainWindow::loadIntoMesh);

	_action_reload = new QAction(tr("&Reload"), this);
	//m_actReload->setIcon(QIcon(":images/fileopen.png"));
	_action_reload->setShortcut(tr("F5"));
	_action_reload->setToolTip(tr("Reloads the active geometry."));
	connect(_action_reload, &QAction::triggered, this, &MainWindow::reloadActiveGeometry);

	_action_reload_all = new QAction(tr("Reload &All"), this);
	//m_actReloadAll->setIcon(QIcon(":images/fileopen.png"));
	_action_reload_all->setShortcut(tr("Ctrl+F5"));
	_action_reload_all->setToolTip(tr("Reloads all geometries."));
	connect(_action_reload_all, &QAction::triggered, this, &MainWindow::reloadAllGeometries);

	_action_save = new QAction(tr("&Save"), this);
	_action_save->setIcon(QIcon(":images/filesave.png"));
	_action_save->setShortcut(tr("Ctrl+S"));
	_action_save->setToolTip(tr("Saves a geometry to a file."));
	connect(_action_save, &QAction::triggered, this, &MainWindow::saveToFile);

	_action_erase = new QAction(tr("&Erase"), this);
	_action_erase->setIcon(QIcon(":images/erase.png"));
	_action_erase->setShortcut(tr("Ctrl+E"));
	_action_erase->setToolTip(tr("erases the selected geometry from the scene."));
	connect(_action_erase, &QAction::triggered, this, &MainWindow::eraseActiveSceneObject);

	// todo remove deprecated
	_action_export_ug3 = new QAction(tr("Export to ug3"), this);
	_action_export_ug3->setToolTip(tr("Exports the geometry to ug3 lgm / ng format."));
	connect(_action_export_ug3, &QAction::triggered, this, &MainWindow::exportToUG3);

	_action_quit = new QAction(tr("Quit"), this);
	connect(_action_quit, &QAction::triggered, this, &MainWindow::quit);

	_menu_file = new QMenu("&File", menuBar());
	_menu_file->addAction(_action_new);
	_menu_file->addAction(_action_open);
	_menu_file->addAction(_action_loac_into_mesh);
	_menu_file->addAction(_action_reload);
	_menu_file->addAction(_action_reload_all);
	_menu_file->addAction(_action_save);
	_menu_file->addAction(_action_erase);
	_menu_file->addSeparator();
	_menu_file->addAction(_action_export_ug3);
	_menu_file->addSeparator();
	_menu_file->addAction(_action_quit);


//	help menu
	_action_help = new QAction(tr("&User Manual"), this);
	_action_help->setShortcut(tr("Ctrl+U"));
	connect(_action_help, &QAction::triggered, this, &MainWindow::showHelp);

	_action_jump_to_script_reference = new QAction(tr("Script and Tools Reference"), this);
	connect(_action_jump_to_script_reference, &QAction::triggered, this, &MainWindow::showScriptReference);

	_action_license = new QAction(tr("License"), this);
	connect(_action_license, &QAction::triggered, this, &MainWindow::showLicense);

	_action_controls = new QAction(tr("Controls"), this);
	connect(_action_controls, &QAction::triggered, this, &MainWindow::showControls);

	_action_shortcuts = new QAction(tr("Shortcuts"), this);
	connect(_action_shortcuts, &QAction::triggered, this, &MainWindow::showShortcuts);

	_action_recent_changes = new QAction(tr("Recent Changes"), this);
	connect(_action_recent_changes, &QAction::triggered, this, &MainWindow::showRecentChanges);

	_action_show_about = new QAction(tr("About"), this);
	connect(_action_show_about, &QAction::triggered, this, &MainWindow::showAbout);

	_action_show_contact = new QAction(tr("Contact"), this);
	connect(_action_show_contact, &QAction::triggered, this, &MainWindow::showContact);


	_menu_help = new QMenu("&Help", menuBar());
	_menu_help->addAction(_action_help);
	_menu_help->addSeparator();
	_menu_help->addAction(_action_controls);
	_menu_help->addAction(_action_shortcuts);
	_menu_help->addAction(_action_jump_to_script_reference);
	_menu_help->addAction(_action_recent_changes);
	_menu_help->addAction(_action_license);
	_menu_help->addAction(_action_show_about);
	_menu_help->addAction(_action_show_contact);


//	create a tool bar for file handling
	QToolBar* fileToolBar = addToolBar(tr("&File"));
	fileToolBar->setObjectName(tr("file_toolbar"));
	fileToolBar->addAction(_action_new);
	fileToolBar->addAction(_action_open);
	fileToolBar->addAction(_action_save);
	fileToolBar->addAction(_action_erase);

//	undo
	auto* actUndo = new QAction(tr("Undo"), fileToolBar);
	actUndo->setIcon(QIcon(":images/editundo.png"));
	actUndo->setShortcut(tr("Ctrl+Z"));
	actUndo->setToolTip(tr("undo"));
	fileToolBar->addAction(actUndo);
	connect(actUndo, &QAction::triggered, this, &MainWindow::undo);

//	redo
	auto actRedo = new QAction(tr("Redo"), fileToolBar);
	actRedo->setIcon(QIcon(":images/editredo.png"));
	actRedo->setShortcut(tr("Ctrl+Z"));
	actRedo->setToolTip(tr("redo"));
	fileToolBar->addAction(actRedo);
	connect(actRedo, &QAction::triggered, this, &MainWindow::redo);

//	create a tool bar for visibility
	createVisibilityToolbar();

//	create the file dialog.
	_dlg_geometry_files = new QFileDialog(this);


//////// DOCK WIDGETS
	setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
	setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);

//	create the scene inspector
	auto* pSceneInspectorDock = new QDockWidget(tr("Scene Inspector"), this);
	pSceneInspectorDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
	pSceneInspectorDock->setObjectName(tr("scene_inspector_dock"));
	_scene_inspector = new SceneInspector(pSceneInspectorDock);
	_scene_inspector->setObjectName(tr("scene_inspector"));
	_scene_inspector->setScene(_scene);

	pSceneInspectorDock->setWidget(_scene_inspector);
	addDockWidget(Qt::RightDockWidgetArea, pSceneInspectorDock);

	connect(_scene_inspector, &SceneInspector::mouseClicked, this, &MainWindow::sceneInspectorClicked);

	connect(_scene_inspector, &SceneInspector::objectChanged, this, &MainWindow::refreshActionLog);

	populateMenuBar ();
	activateModule(new MeshModule(this));

	resize(settings().value("mainWindow/size", QSize(1024, 768)).toSize());
	move(settings().value("mainWindow/pos", QPoint(10, 10)).toPoint());
	restoreState(settings().value("mainWindow/windowState").toByteArray());

	_p_log->raise();
	
//	init the status bar
	// statusBar()->show();

//	init undo
	QDir tmpPath = app::ProMeshTmpDir();
	if(!UndoHistoryProvider::inst().init(tmpPath.path().toLocal8Bit().constData())){
		cout << "initialization of undo failed. couldn't create history path at "
			 << tmpPath.path().toLocal8Bit().constData() << "\n";
	}

	show();
	check_options();
}

void MainWindow::check_options() const
{
	const opts::Options& o = GetOptions();

	if(!o._undo._enabled) {
		UG_LOG("OPTIONS WARNING: Undo is disabled!\n");
	}
}



QToolBar* MainWindow::createVisibilityToolbar()
{
	QToolBar* visToolBar = addToolBar(tr("&Visibility"));
	visToolBar->setObjectName(tr("visibility_toolbar"));

//	layer for front:
	auto* lblFront = new QLabel(visToolBar);
	lblFront->setText(tr(" front: "));
	visToolBar->addWidget(lblFront);

//	combo-box for front-render mode.
	auto* visFront = new QComboBox(visToolBar);
	visToolBar->addWidget(visFront);
	visFront->addItem(tr("none"));
	visFront->addItem(tr("wire"));
	visFront->addItem(tr("solid"));
	visFront->addItem(tr("solid + wire"));

//	connect signals and slots
	connect(visFront, &QComboBox::currentIndexChanged, this, &MainWindow::frontDrawModeChanged);

//	init the value
	visFront->setCurrentIndex(3);

//	seperator
	visToolBar->addSeparator();

//	layer for back:
	auto* lblBack = new QLabel(visToolBar);
	lblBack->setText(tr(" back: "));
	visToolBar->addWidget(lblBack);

//	combo-box for front-render mode.
	auto* visBack = new QComboBox(visToolBar);
	visToolBar->addWidget(visBack);
	visBack->addItem(tr("none"));
	visBack->addItem(tr("wire"));
	visBack->addItem(tr("solid"));
	visBack->addItem(tr("solid + wire"));

//	connect signals and slots
	connect(visBack, &QComboBox::currentIndexChanged, this, &MainWindow::backDrawModeChanged);

//	init the value
	visBack->setCurrentIndex(3);


//	add a combo-box for the element-mode
/*
	visToolBar->addSeparator();
	QComboBox* elemMode = new QComboBox(visToolBar);
	visToolBar->addWidget(elemMode);
	elemMode->addItem(tr("draw vertices"));
	elemMode->addItem(tr("draw edges"));
	elemMode->addItem(tr("draw faces"));
	elemMode->addItem(tr("draw volumes"));
	elemMode->setCurrentIndex(3);
	m_elementModeListIndex = 3;

//	connect signals and slots
	connect(elemMode, SIGNAL(currentIndexChanged(int)), this, &MainWindow::elementDrawModeChanged(int)));
*/
	visToolBar->addSeparator();
	_tb_render_vrts = new QToolButton(visToolBar);
	_tb_render_vrts->setIcon(QIcon(":images/icon_render_vertices.png"));
	_tb_render_vrts->setCheckable(true);
	_tb_render_vrts->setChecked(true);
	_tb_render_vrts->setAutoExclusive(false);
	_tb_render_vrts->setToolTip(tr("render vertices"));
	visToolBar->addWidget(_tb_render_vrts);
	connect(_tb_render_vrts, &QToolButton::toggled, this, &MainWindow::elementDrawModeChanged);

	_tb_render_edges = new QToolButton(visToolBar);
	_tb_render_edges->setIcon(QIcon(":images/icon_render_edges.png"));
	_tb_render_edges->setCheckable(true);
	_tb_render_edges->setChecked(true);
	_tb_render_edges->setAutoExclusive(false);
	_tb_render_edges->setToolTip(tr("render edges"));
	visToolBar->addWidget(_tb_render_edges);
	connect(_tb_render_edges, &QToolButton::toggled, this, &MainWindow::elementDrawModeChanged);

	_tb_render_faces = new QToolButton(visToolBar);
	_tb_render_faces->setIcon(QIcon(":images/icon_render_faces.png"));
	_tb_render_faces->setCheckable(true);
	_tb_render_faces->setChecked(true);
	_tb_render_faces->setAutoExclusive(false);
	_tb_render_faces->setToolTip(tr("render faces"));
	visToolBar->addWidget(_tb_render_faces);
	connect(_tb_render_faces, &QToolButton::toggled, this, &MainWindow::elementDrawModeChanged);

	_tb_render_vols = new QToolButton(visToolBar);
	_tb_render_vols->setIcon(QIcon(":images/icon_render_volumes.png"));
	_tb_render_vols->setCheckable(true);
	_tb_render_vols->setChecked(true);
	_tb_render_vols->setAutoExclusive(false);
	_tb_render_vols->setToolTip(tr("render volumes"));
	visToolBar->addWidget(_tb_render_vols);
	connect(_tb_render_vols, &QToolButton::toggled, this, &MainWindow::elementDrawModeChanged);


//	add a combo-box for the selection elements
/*
	visToolBar->addSeparator();
	QComboBox* selElems = new QComboBox(visToolBar);
	visToolBar->addWidget(selElems);
	selElems->addItem(tr("select vertices"));
	selElems->addItem(tr("select edges"));
	selElems->addItem(tr("select faces"));
	selElems->addItem(tr("select volumes"));

//	connect signals and slots
	connect(selElems, &QComboBox::currentIndexChanged, this, &MainWindow::selectionElementChanged(int)));

//	init the value
	selElems->setCurrentIndex(0);
*/

	visToolBar->addSeparator();
	_tb_sel_vrts = new QToolButton(visToolBar);
	_tb_sel_vrts->setIcon(QIcon(":images/icon_vertices.png"));
	_tb_sel_vrts->setCheckable(true);
	_tb_sel_vrts->setAutoExclusive(true);
	_tb_sel_vrts->setToolTip(tr("select vertices"));
	visToolBar->addWidget(_tb_sel_vrts);
	connect(_tb_sel_vrts, &QToolButton::toggled, this, static_cast<void (MainWindow::*)(int)>(&MainWindow::selectionElementChanged));

	_tb_sel_edges = new QToolButton(visToolBar);
	_tb_sel_edges->setIcon(QIcon(":images/icon_edges.png"));
	_tb_sel_edges->setCheckable(true);
	_tb_sel_edges->setAutoExclusive(true);
	_tb_sel_edges->setToolTip(tr("select edges"));
	visToolBar->addWidget(_tb_sel_edges);
	connect(_tb_sel_edges, &QToolButton::toggled, this, static_cast<void (MainWindow::*)(int)>(&MainWindow::selectionElementChanged));

	_tb_sel_faces = new QToolButton(visToolBar);
	_tb_sel_faces->setIcon(QIcon(":images/icon_faces.png"));
	_tb_sel_faces->setCheckable(true);
	_tb_sel_faces->setAutoExclusive(true);
	_tb_sel_faces->setToolTip(tr("select faces"));
	visToolBar->addWidget(_tb_sel_faces);
	connect(_tb_sel_faces, &QToolButton::toggled, this, static_cast<void (MainWindow::*)(int)>(&MainWindow::selectionElementChanged));

	_tb_sel_vols = new QToolButton(visToolBar);
	_tb_sel_vols->setIcon(QIcon(":images/icon_volumes.png"));
	_tb_sel_vols->setCheckable(true);
	_tb_sel_vols->setAutoExclusive(true);
	_tb_sel_vols->setToolTip(tr("select volumes"));
	visToolBar->addWidget(_tb_sel_vols);
	connect(_tb_sel_vols, &QToolButton::toggled, this,static_cast<void (MainWindow::*)(int)>(&MainWindow::selectionElementChanged));

	if(!_tb_sel_vrts->isChecked())
		_tb_sel_vrts->toggle();

//	add a combo-box for the selection modes
	visToolBar->addSeparator();
	_sel_modes = new QComboBox(visToolBar);
	visToolBar->addWidget(_sel_modes);
	_sel_modes->addItem(QIcon(":images/icon_click_select.png"), tr(""));
	_sel_modes->addItem(QIcon(":images/icon_box_select_cut.png"), tr(""));
	_sel_modes->addItem(QIcon(":images/icon_box_select.png"), tr(""));

//	connect signals and slots
	connect(_sel_modes, &QComboBox::currentIndexChanged, this, &MainWindow::selectionModeChanged);

//	init the value
	_sel_modes->setCurrentIndex(0);



//	add a color-picker for the background color
	visToolBar->addSeparator();

//	layer for color:
	auto* lblColor = new QLabel(visToolBar);
	lblColor->setText(tr(" bg-color: "));
	visToolBar->addWidget(lblColor);

	_bg_color = new ColorWidget(visToolBar);
	visToolBar->addWidget(_bg_color);
	connect(_bg_color, &ColorWidget::colorChanged, this, &MainWindow::backgroundColorChanged);

	_bg_color->setFixedWidth(24);
	_bg_color->setFixedHeight(24);

	QString strDefColor("#666666");
	if(settings().contains("bg-color")){
		QVariant value = settings().value("bg-color", strDefColor);
		_bg_color->setColor(QColor(value.toString()));
	}
	else
		_bg_color->setColor(QColor(strDefColor));


	//	world scale
	visToolBar->addSeparator();
	visToolBar->addWidget(new QLabel(tr(" View-scale")));
	visToolBar->addWidget(new QLabel(tr(" x:")));
	_view_scale_x = new TruncatedDoubleSpinBox(visToolBar);
	_view_scale_x->setValue(1);
	_view_scale_x->setSingleStep(0.1);
	_view_scale_x->setMinimum(-1.e6);
	_view_scale_x->setMaximum(1.e6);
	_view_scale_x->setFixedWidth(32);
	connect(_view_scale_x, &TruncatedDoubleSpinBox::valueChanged, this, &MainWindow::viewScaleXChanged);
	visToolBar->addWidget(_view_scale_x);

	visToolBar->addWidget(new QLabel(tr(" y:")));
	_view_scale_y = new TruncatedDoubleSpinBox(visToolBar);
	_view_scale_y->setValue(1);
	_view_scale_y->setSingleStep(0.1);
	_view_scale_y->setMinimum(-1.e6);
	_view_scale_y->setMaximum(1.e6);
	_view_scale_y->setFixedWidth(32);
	connect(_view_scale_y, &TruncatedDoubleSpinBox::valueChanged, this, &MainWindow::viewScaleYChanged);
	visToolBar->addWidget(_view_scale_y);

	visToolBar->addWidget(new QLabel(tr(" z:")));
	_view_scale_z = new TruncatedDoubleSpinBox(visToolBar);
	_view_scale_z->setValue(1);
	_view_scale_z->setSingleStep(0.1);
	_view_scale_z->setMinimum(-1.e6);
	_view_scale_z->setMaximum(1.e6);
	_view_scale_z->setFixedWidth(32);
	connect(_view_scale_z, &TruncatedDoubleSpinBox::valueChanged, this, &MainWindow::viewScaleZChanged);
	visToolBar->addWidget(_view_scale_z);

	return visToolBar;
}

uint MainWindow::getLGElementMode()
{
	switch(_element_mode_list_index){
		case 0: return LGEM_VERTEX;
		case 1: return LGEM_EDGE;
		case 2: return LGEM_FACE;
		case 3: return LGEM_VOLUME;
		default: return LGEM_NONE;
	}
}

bool MainWindow::load_grid_from_file(const char* filename)
{
	try{
		LOG("loading " << filename << " ...\n");
		LGObject* pObj = CreateLGObjectFromFile(filename);

	//	add it to the scene
		if(pObj)
		{
			const bool bFirstLoad = _scene->num_objects() == 0;
			pObj->set_element_mode(getLGElementMode());
			int index = _scene->add_object(pObj);
			if(index != -1)
			{
				setActiveObject(index);

			//	if this is the first object loaded, we will focus it.
				if(bFirstLoad)
				{
					Sphere3 s = pObj->get_bounding_sphere();
					_p_view->fly_to(cam::Vector3(s.get_center().x(),
													s.get_center().y(),
													s.get_center().z()),
									s.get_radius() * 3.f);
				}

				pObj->set_save_required(false);
				return true;
			}
		}
	}
	catch(UGError err){
		UG_LOG("ERROR: " << err.get_msg() << endl);
		return false;
	}
	catch(std::runtime_error &err){
		UG_LOG("ERROR: " << err.what() << endl);
		return false;
	}

	return false;
}

LGObject* MainWindow::create_empty_object(const char* name, SceneObjectType sot)
{
//	create a new object
	LGObject* pObj = nullptr;

	switch(sot){
		case SOT_LG:
    		pObj = CreateEmptyLGObject(name);
    		break;

    	case SOT_CSG:
    		pObj = CreateEmptyCSGObject(name);
	}

	UG_COND_THROW(!pObj, "Invalid SceneObjectType specified!");

    pObj->set_element_mode(getLGElementMode());

//	add it to the scene
	int index = _scene->add_object(pObj);
	if(index != -1)
		setActiveObject(index);

	pObj->geometry_changed();
	return pObj;
}

bool MainWindow::save_object_to_file(ISceneObject* obj, const char* filename)
{
	auto* lgobj = dynamic_cast<LGObject*>(obj);
	if(lgobj){
		try{
			lgobj->set_save_required(false);
			return SaveLGObjectToFile(lgobj, filename);
		}
		catch(UGError err){
			UG_LOG("ERROR: " << err.get_msg() << endl);
			return false;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////
//	public slots
void MainWindow::newGeometry()
{
	create_empty_object("newObject", SOT_LG);
}

int MainWindow::openFile()
{
	int numOpened = 0;

	QString path = settings().value("file-path", ".").toString();

	QStringList fileNames = QFileDialog::getOpenFileNames(
								this,
								tr("Load Geometry"),
								path,
								tr("geometry files (").append(LG_SUPPORTED_FILE_FORMATS_OPEN).append(")"));

	for(QStringList::iterator iter = fileNames.begin();
		iter != fileNames.end(); ++iter)
	{
		settings().setValue("file-path", QFileInfo(*iter).absolutePath());
	//	load the object
		if(load_grid_from_file((*iter).toLocal8Bit().constData()))
			++numOpened;
		else
		{
			QMessageBox msg(this);
			QString str = tr("Load failed: ");
			str.append(*iter);
			msg.setText(str);
			msg.exec();
		}
	}

	return numOpened;
}


int MainWindow::loadIntoMesh()
{
	int numOpened = 0;

	QString path = settings().value("file-path", ".").toString();

	QStringList fileNames = QFileDialog::getOpenFileNames(
								this,
								tr("Load Geometry"),
								path,
								tr("geometry files (").append(LG_SUPPORTED_FILE_FORMATS_OPEN).append(")"));

	LGObject* pObj = getActiveObject();
	if(!pObj)
		pObj = create_empty_object("newObject", SOT_LG);

	for(QStringList::iterator iter = fileNames.begin();
		iter != fileNames.end(); ++iter)
	{
		settings().setValue("file-path", QFileInfo(*iter).absolutePath());
	//	load the object
		try{
			LoadLGObjectFromFile(pObj, (*iter).toLocal8Bit().constData(), false);
			++numOpened;
		}
		catch(UGError err){
			delete pObj;
			pObj = nullptr;
			QMessageBox msg(this);
			QString str = tr("Load failed: ");
			str.append(err.get_msg().c_str());
			msg.setText(str);
			msg.exec();
			break;
		}
	}

//	add it to the scene
	if(pObj)
	{
		PerformLoadPostprocessing(pObj);
		const bool bFirstLoad = _scene->num_objects() == 0;
		pObj->set_element_mode(getLGElementMode());
		int index = _scene->add_object(pObj);
		if(index != -1)
		{
			setActiveObject(index);

		//	if this is the first object loaded, we will focus it.
			if(bFirstLoad)
			{
				Sphere3 s = pObj->get_bounding_sphere();
				_p_view->fly_to(cam::Vector3(s.get_center().x(),
												s.get_center().y(),
												s.get_center().z()),
								s.get_radius() * 3.f);
			}

			pObj->set_save_required(false);
		}
	}

	return numOpened;
}


bool MainWindow::reloadActiveGeometry()
{
	LGObject* obj = app::getActiveObject();
	if(obj){
		bool success = ReloadLGObject(obj);
		obj->set_save_required(false);
		return success;
	}
	return false;
}

bool MainWindow::reloadAllGeometries()
{
	LGScene* scene = app::getActiveScene();
	if(scene){
		bool success = true;
		for (int i = 0; i < scene->num_objects(); ++i){
			success &= ReloadLGObject(scene->get_object(i));
			scene->get_object(i)->set_save_required(false);
		}
		return success;
	}
	return false;
}

bool MainWindow::saveToFile()
{
	bool saveFailed = false;
	ISceneObject* obj = _scene_inspector->getActiveObject();
	if(obj)
	{
		QString path = settings().value("file-path", ".").toString();
		path.append("/").append(obj->name()).append(".ugx");

		QString fileName = QFileDialog::getSaveFileName(
									this,
									tr("Save Geometry"),
									path,
									tr("geometry files (").append(LG_SUPPORTED_FILE_FORMATS_SAVE).append(")"));

		if(!fileName.isEmpty())
		{
			settings().setValue("file-path", QFileInfo(fileName).absolutePath());
		//	save the object
			if(!save_object_to_file(obj, fileName.toLocal8Bit().constData()))
				saveFailed = true;
			else{
				obj->set_name(QFileInfo(fileName).baseName().toLocal8Bit().constData());
				obj->visuals_changed();
				auto* lgobj = dynamic_cast<LGObject*>(obj);
				if(lgobj)
					lgobj->set_save_required(false);
			}
		}
	}

	if(saveFailed)
	{
		QMessageBox msg(this);
		QString str = tr("Save failed: ");
		if(!obj)
			str.append("no object selected.");
		else
		{
			str.append("make sure that the selected filename has a valid suffix.\n");
			str.append("valid suffixes are: *.ugx *.ncdf *.lgb *.obj *.txt *.ele");
		}
		msg.setText(str);
		msg.exec();
		return false;
	}

	return true;
}

bool MainWindow::exportToUG3()
{
	bool saveFailed = false;

	auto obj = dynamic_cast<LGObject*>(_scene_inspector->getActiveObject());
	if(obj)
	{

		QString path = settings().value("file-path", ".").toString();

		QString fileName = QFileDialog::getSaveFileName(
									this,
									tr("Save Geometry"),
									path);
		try{
			if(!fileName.isEmpty())
			{
				settings().setValue("file-path", QFileInfo(fileName).absolutePath());

			//	get the filenames
				QFileInfo fileInfo(fileName);
				QString prefix = fileInfo.absolutePath();
				prefix.append("/").append(fileInfo.baseName());
				Grid& g = obj->grid();
				SubsetHandler& sh = obj->subset_handler();

				if(g.num_volumes() > 0){
				//	Create the subset-handlers
					SubsetHandler shFaces(g, SHE_FACE);
					SubsetHandler shVolumes(g, SHE_VOLUME);

					for(int i = 0; i < sh.num_subsets(); ++i){
						shFaces.assign_subset(sh.begin<Face>(i), sh.end<Face>(i), i);
						shVolumes.assign_subset(sh.begin<Volume>(i), sh.end<Volume>(i), i);
					}

					UG_LOG("Exporting to UG3 3D ... ");
				//	export the grid
					saveFailed = !ExportGridToUG(g, shFaces, shVolumes, prefix.toLocal8Bit().constData(),
												"tmpLGMName", "tmpProblemName", 0);
				}
				else{
					UG_LOG("Exporting to UG3 2D ... ");
				//	export the grid
					saveFailed = !ExportGridToUG_2D(g, prefix.toLocal8Bit().constData(),
													"tmpLGMName", "tmpProblemName", 0, &sh);
				}

				if(saveFailed){
					UG_LOG("failed\n");
				}
				else{
					UG_LOG("done\n");
				}
			}
		}
		catch(UGError err){
			UG_LOG("\n");
			UG_LOG("ERROR: " << err.get_msg() << endl);
		}
	}

	if(saveFailed)
	{
		QMessageBox msg(this);
		QString str = tr("Export failed: ");
		str.append(tr("Make sure that all subsets are consecutive. That means:\n"));
		str.append(tr("  * A subset that contains no faces may not be followed by a subset that contains faces.\n"));
		str.append(tr("  * A subset that contains no volumes may not be followed by a subset that contains volumes.\n"));
		str.append(tr("  Consider calling Tools->Subsets->Adjust Subsets For UG3\n"));
		msg.setText(str);
		msg.exec();
		return false;
	}

	return true;
}

void MainWindow::eraseActiveSceneObject()
{
//	get the active object from the scene-inspector
	ISceneObject* obj = _scene_inspector->getActiveObject();
	if(obj)
	{
	//	get the objects index and erase it from the scene
		int index = _scene->get_object_index(obj);
		if(index >= 0)
		{
			bool performErase = true;

			auto lgobj = dynamic_cast<LGObject*>(obj);

			if(!lgobj || lgobj->save_required()){
				QString msg = QString("Erase '").append(obj->name()).append("'?\n").
												append("No undo will be possible!");
				QMessageBox::StandardButton reply;
				reply = QMessageBox::question(this, "Erase?", msg,
											  QMessageBox::Yes | QMessageBox::No);
				performErase = (reply == QMessageBox::Yes);
			}

			if(performErase){
				if(lgobj == _action_log_sender)
					_action_log_sender = nullptr;

			//	perform erase
				_scene->erase_object(index);
			//	select the next object
				if(index < _scene->num_objects())
					setActiveObject(index);
				else if(index > 0)
					setActiveObject(index - 1);
			}
		}
	}
}

LGObject* MainWindow::getActiveObject()
{
	return dynamic_cast<LGObject*>(_scene_inspector->getActiveObject());
}

void MainWindow::setActiveObject(int index)
{
	_scene_inspector->setActiveObject(index);
	if(getActiveObject() != _active_object){
		_active_object = getActiveObject();
		emit activeObjectChanged();
	}
}

void MainWindow::launchHelpBrowser(const QString& pageName)
{
	#ifdef PROMESH_USE_WEBKIT
		if(!m_helpBrowser)
			m_helpBrowser = new QHelpBrowser(this);
		m_helpBrowser->browse(QUrl(QString("qrc:///docs/").append(pageName)));
		QRect geom = this->geometry();
		geom.adjust(50, 50, -50, -50);
		m_helpBrowser->setGeometry(geom);
		m_helpBrowser->show();
	#else
		try{
		//todo:	one could compare the version.txt files in the resource and the target
		//		folder and only copy if they do not match.
			QString helpHtmlPath = app::UserHelpDir().path().append("/docs");
			static bool firstRun = true;
			if(firstRun){
				firstRun = false;
				// try{
					if(FileExists(helpHtmlPath))
						EraseDirectory(helpHtmlPath);
					CopyDirectory(":/docs", app::UserHelpDir().path());
				// }
				// catch(const UGError& err){
				// 	UG_LOG("ERROR: " << err.get_msg() << endl);
				// 	UG_LOG("WARNING: Help may be outdated." << endl);
				// }
			}

			QUrl url;
			if(helpHtmlPath.at(0) == '/')
				url = QUrl(QString("file://") + helpHtmlPath + "/" + pageName);
			else
				url = QUrl(QString("file:///") + helpHtmlPath + "/" + pageName);
			QDesktopServices::openUrl(url);
		}
		catch(UGError& err){
			UG_LOG("ERROR: " << err.get_msg() << endl);
		}
	#endif
}

void MainWindow::showHelp()
{
	launchHelpBrowser("index.html");
}

void MainWindow::showRecentChanges()
{
	launchHelpBrowser("pageRecentChanges.html");
}

void MainWindow::showControls()
{
	launchHelpBrowser("pageControls.html");
}

void MainWindow::showShortcuts()
{
	launchHelpBrowser("pageShortcuts.html");
}

void MainWindow::showLicense()
{
	launchHelpBrowser("pageProMeshLicense.html");
}

void MainWindow::showScriptReference()
{
	launchHelpBrowser("modules.html");
}

void MainWindow::showAbout()
{
	launchHelpBrowser("pageAbout.html");
}

void MainWindow::showContact()
{
	launchHelpBrowser("pageContact.html");
}

void MainWindow::frontDrawModeChanged(int newMode)
{
	_scene->set_draw_mode_front(newMode);
}

void MainWindow::backDrawModeChanged(int newMode)
{
	_scene->set_draw_mode_back(newMode);
}

void MainWindow::backgroundColorChanged(const QColor& color)
{
	_p_view->set_background_color(color);
	settings().setValue("bg-color", color.name());
}

void MainWindow::selectionElementChanged(int newElement)
{
	_selection_element = newElement;
}

void MainWindow::selectionElementChanged(bool)
{
	if(_tb_sel_vrts->isChecked())
		_selection_element = 0;
	else if(_tb_sel_edges->isChecked())
		_selection_element = 1;
	else if(_tb_sel_faces->isChecked())
		_selection_element = 2;
	else if(_tb_sel_vols->isChecked())
		_selection_element = 3;
}

void MainWindow::selectionModeChanged(int newMode)
{
	_selection_mode = newMode;
}

void MainWindow::elementDrawModeChanged()
{

	_scene->set_element_draw_mode(_tb_render_vrts->isChecked(), _tb_render_edges->isChecked(),
								   _tb_render_faces->isChecked(), _tb_render_vols->isChecked());
	_scene->update_visuals();
}

////////////////////////////////////////////////////////////////////////
//	events
void MainWindow::closeEvent(QCloseEvent *event)
{
	if(_scene->num_objects() == 0)
		event->accept();
	else{
		QMessageBox::StandardButton reply = QMessageBox::question(this,
			"Quit?", "Quit? Unsaved progress will be lost!",
			QMessageBox::Yes | QMessageBox::No);

		if (reply == QMessageBox::Yes){
			QMainWindow::closeEvent(event);
			event->accept();
		}
		else{
			event->ignore();
		}
	}

	if(event->isAccepted()){
		// settings().setValue("mainWindow/geometry", saveGeometry());
		settings().setValue("mainWindow/size", size());
    	settings().setValue("mainWindow/pos", pos());
		settings().setValue("mainWindow/windowState", saveState());
	}
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
	event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event)
{
	QList<QUrl> urls = event->mimeData()->urls();
	if (urls.isEmpty())
	   return;

	for(auto iter = urls.begin(); iter != urls.end(); ++iter)
	{
		settings().setValue("file-path", QFileInfo((*iter).toLocalFile()).absolutePath());
		if(!load_grid_from_file((*iter).toLocalFile().toLatin1().constData()))
		{
			QMessageBox msg(this);
			QString str = tr("Load failed: ");
			str.append((*iter).toLocalFile());
			msg.setText(str);
			msg.exec();
		}
	}
}

void MainWindow::sceneInspectorClicked(QMouseEvent* event)
{
	if(getActiveObject() != _active_object){
		_active_object = getActiveObject();
		emit activeObjectChanged();
	}

	if(event->button() == Qt::RightButton){
		if(_scene_inspector_r_click_menu)
			_scene_inspector_r_click_menu->exec(QCursor::pos());
	}
}


void MainWindow::undo()
{
	LGObject* obj = app::getActiveObject();
	if(obj){
		if(!obj->undo()){
			UG_LOG("no more steps to undo.\n");
		}
	}
}

void MainWindow::redo()
{
	LGObject* obj = app::getActiveObject();
	if(obj){
		if(!obj->redo()){
			UG_LOG("no more steps to redo.\n");
		}
	}
}

void MainWindow::quit()
{
	this->close();
}

void MainWindow::
viewScaleXChanged(double value)
{
	vector3 ws = _p_view->camera().world_scale();
	ws.x() = value;
	_p_view->camera().set_world_scale(ws);
	_scene->update_visuals();
}

void MainWindow::
viewScaleYChanged(double value)
{
	vector3 ws = _p_view->camera().world_scale();
	ws.y() = value;
	_p_view->camera().set_world_scale(ws);
	_scene->update_visuals();
}

void MainWindow::
viewScaleZChanged(double value)
{
	vector3 ws = _p_view->camera().world_scale();
	ws.z() = value;
	_p_view->camera().set_world_scale(ws);
	_scene->update_visuals();
}


void MainWindow::
refreshOptions()
{
	_opt_widget->populate(&GetOptions(), "options");
}

void MainWindow::
optionsChanged()
{
	_opt_widget->retrieve_values(GetOptions());
	saveOptions();
}

void
MainWindow::
saveOptions()
{
	string filename = app::UserDataDir().path().toStdString().
			append("/config.xml");

	ofstream out(filename.c_str());
	boost::archive::xml_oarchive ar(out);

	ar & make_nvp("config", GetOptions());
}

void MainWindow::
loadOptions()
{
	QString userOptsName = app::UserDataDir().path().append("/config.xml");
	if(!FileExists(userOptsName)){
		QFile::copy(":/resources/config.xml", userOptsName);
	}

	if(FileExists(userOptsName)){
		ifstream in(userOptsName.toStdString().c_str());
		boost::archive::xml_iarchive ar(in);

		ar & make_nvp("config", GetOptions());
	}

	refreshOptions();
}


void MainWindow::
populateMenuBar()
{
	QMenuBar* bar = menuBar();
	bar->clear();
	bar->addMenu(_menu_file);

	for(auto i = _module_menus.begin(); i != _module_menus.end(); ++i)
	{
		bar->addMenu(*i);
	}

	bar->addMenu(_menu_help);
}

void MainWindow::
activateModule(IModule* mod)
{

	if(_active_module == mod)
		return;

	if(_active_module){
	//	remove old modules dock widgets
		for(auto i = _module_dock_widgets.begin();
		    i != _module_dock_widgets.end(); ++i)
		{
			removeDockWidget(i->second);
		}

		_active_module->deactivate();
	}
	
	_active_module = mod;

	if(_active_module){
		_active_module->activate (_scene_inspector, _scene);
		_scene_inspector_r_click_menu = mod->getSceneInspectorMenu ();
		_module_dock_widgets = mod->getDockWidgets ();
		_module_menus = mod->getMenus ();

		populateMenuBar();

	//	add dock widgets
		for(auto i = _module_dock_widgets.begin(); i != _module_dock_widgets.end(); ++i)
		{
			addDockWidget(i->first, i->second);
		}
	}
}


void MainWindow::
refreshActionLog(ISceneObject* iobj)
{
	auto* obj = dynamic_cast<LGObject*>(iobj);
	if(obj){
		_action_log->setPlainText(obj->action_log());
		if(_action_log_sender){
			disconnect(_action_log_sender, &LGObject::actionLogChanged, this, &MainWindow::actionLogChanged);
			disconnect(_action_log_sender, &LGObject::actionLogCleared, this, &MainWindow::actionLogCleared);
		}
		_action_log_sender = obj;
		connect(obj, &LGObject::actionLogChanged, this, &MainWindow::actionLogChanged);
		connect(obj, &LGObject::actionLogCleared, this, &MainWindow::actionLogCleared);
	}
}


void MainWindow::
actionLogChanged(const QString& newContent)
{
	auto obj = dynamic_cast<LGObject*>(sender());
	if(obj == _action_log_sender){
			_action_log->moveCursor (QTextCursor::End);
			_action_log->insertPlainText (newContent);
			_action_log->moveCursor (QTextCursor::End);
		// }
	}
}

void MainWindow::
actionLogCleared()
{
	_action_log->setPlainText("");
}

const char* MainWindow::log_text()
{
	return _p_log_text->toPlainText().toLocal8Bit().constData();
}
