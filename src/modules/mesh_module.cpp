/*
 * Copyright (c) 2017:  G-CSC, Goethe University Frankfurt
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

#include <QApplication>
#include <QDesktopServices>
#include <QDockWidget>
#include <QFileDialog>
#include <QKeyEvent>
#include <QMessageBox>

#include "app.hpp"

#include "clip_plane_widget.hpp"
#include "mesh_module.hpp"
#include "rclick_menu_scene_inspector.hpp"
#include "scene_inspector.hpp"
#include "scene/lg_scene.hpp"
#include "tools/standard_tools.hpp"
#include "tools/tool_manager.hpp"
#include "widgets/projector_widget.hpp"
#include "widgets/property_widget.hpp"
#include "widgets/tool_browser_widget.hpp"
#include "widgets/widget_list.hpp"
#include "widgets/matrix_widget.hpp"
#include "tools/coordinate_transform_tools.h"
#include "widgets/script_editor.hpp"
#include "util/file_util.hpp"

using namespace std;
using namespace ug;



MeshModule::
MeshModule (QWidget* parent) :
	IModule(parent),
	_scene_inspector(nullptr),
	_scene(nullptr),
	_scene_inspector_menu(nullptr)
{}



void MeshModule::
activate(SceneInspector* scene_inspector, LGScene* scene)
{
	if(_scene_inspector == scene_inspector && _scene == scene)
		return;
	
	if(_scene_inspector || _scene){
		deactivate();
		if(_scene_inspector_menu)
			delete _scene_inspector_menu;
	}

	_scene_inspector = scene_inspector;

	if(_dock_widgets.empty()){
	//	projector dock
		auto pProjectorDock = new QDockWidget(tr("Projectors"), parentWidget());
		pProjectorDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
		pProjectorDock->setObjectName(tr("projector_dock"));

		auto* projectorList = new WidgetList(pProjectorDock);
		_widget_projector = new ProjectorWidget(projectorList);
		projectorList->addWidget(_widget_projector);

		pProjectorDock->setWidget(projectorList);
		_dock_widgets.emplace_back(Qt::RightDockWidgetArea, pProjectorDock);


	//	clip-plane dock
		auto* pClipPlaneDock= new QDockWidget(tr("Clip Planes"), parentWidget());
		pClipPlaneDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
		pClipPlaneDock->setObjectName(tr("clip_plane_widget_dock"));
		_widget_clip_plane = new ClipPlaneWidget(pClipPlaneDock);
		pClipPlaneDock->setWidget(_widget_clip_plane);
		_dock_widgets.emplace_back(Qt::RightDockWidgetArea, pClipPlaneDock);

		_tool_manager = new ToolManager(parentWidget());
		try{
			RegisterStandardTools(_tool_manager);
		}
		catch(UGError& err){
			UG_LOG("ERROR: ")
			for(size_t i = 0; i < err.num_msg(); ++i){
				if(i > 0){
					UG_LOG("       ");
				}
				UG_LOG(err.get_msg(i) << endl);
			}
			UG_LOG("------------------------------------------------------------------------------------------\n")
		}


	//	tool browser dock
		auto* toolBrowserDock = new QDockWidget(tr("Tool Browser"), parentWidget());
		toolBrowserDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
		toolBrowserDock->setObjectName(tr("tool_browser_dock"));

		_tool_browser = new ToolBrowser(parentWidget());
		_tool_browser->refresh(_tool_manager);
		_tool_browser->setObjectName(tr("tool_browser"));
		toolBrowserDock->setWidget(_tool_browser);
		_dock_widgets.emplace_back(Qt::LeftDockWidgetArea, toolBrowserDock);


	//	coordinate dock
		auto* coordinatesDock = new QDockWidget(tr("Coordinates"), parentWidget());
		coordinatesDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
		coordinatesDock->setObjectName(tr("coordinates_dock"));
		const char* coordLabels[] = {"x", "y", "z"};
		_widget_coords = new MatrixWidget(3, 1, coordinatesDock, coordLabels, true);
		connect(scene, &LGScene::geometry_changed, this, &MeshModule::refreshCoordinates);
		connect(scene, &LGScene::selection_changed, this, &MeshModule::refreshCoordinates);
		connect(_scene_inspector, &SceneInspector::objectChanged, this, &MeshModule::refreshCoordinates);
		connect(_widget_coords, static_cast<void (MatrixWidget::*)()>(&MatrixWidget::valueChanged), this, &MeshModule::coordinatesChanged);

		_widget_coords->setFixedHeight(_widget_coords->sizeHint().height());
		coordinatesDock->setWidget(_widget_coords);
		_dock_widgets.emplace_back(Qt::LeftDockWidgetArea, coordinatesDock);

	//	script editor
		_script_editor = new QScriptEditor(parentWidget());
		_script_editor->setWindowFlags(Qt::Window |
		                               Qt::CustomizeWindowHint |
		                               Qt::WindowCloseButtonHint |
		                               Qt::WindowMinimizeButtonHint |
		                               Qt::WindowMaximizeButtonHint);

	//	script menu
		auto* action_script_editor = new QAction(tr("Live Script Editor"), parentWidget());
		action_script_editor->setStatusTip("Opens the live script editor");
		connect(action_script_editor, &QAction::triggered, this, &MeshModule::showScriptEditor);

		auto* action_new_script = new QAction(tr("New Script"), parentWidget());
		action_new_script->setStatusTip("Creates a new script and opens it for editing");
		connect(action_new_script, &QAction::triggered, this, &MeshModule::newScript);

		auto* action_edit_script = new QAction(tr("Edit Script"), parentWidget());
		action_edit_script->setStatusTip("Opens a script for editing");
		connect(action_edit_script, &QAction::triggered, this, &MeshModule::editScript);

		auto* action_browse_user_scripts = new QAction(tr("Browse Default User Scripts"), parentWidget());
		action_browse_user_scripts->setStatusTip("Opens the default path at which user scripts are located.");
		connect(action_browse_user_scripts, &QAction::triggered, this, &MeshModule::browseUserScripts);

		auto* action_refresh_tool_dialogs = new QAction(tr("Refresh Tool Dialogs"), parentWidget());
		action_refresh_tool_dialogs->setShortcut(tr("Ctrl+T"));
		action_refresh_tool_dialogs->setStatusTip("Refreshes contents of the tool-dialogs.");
		connect(action_refresh_tool_dialogs, &QAction::triggered, this, &MeshModule::refreshToolDialogsClicked);

		auto* action_add_custom_user_script_dir = new QAction(tr("Add Custom User Script Directory"), parentWidget());
		action_add_custom_user_script_dir->setStatusTip("Adds Custom User Scripts located in the specified folder to the tool-dialog.");
		connect(action_add_custom_user_script_dir, &QAction::triggered, this, &MeshModule::addCustomUserScriptDir);

		auto* action_remove_custom_user_script_dirs = new QAction(tr("Remove Custom User Script Directories"), parentWidget());
		action_remove_custom_user_script_dirs->setStatusTip("Removes Custom User Scripts from the tool-dialog.");
		connect(action_remove_custom_user_script_dirs, &QAction::triggered, this, &MeshModule::removeCustomUserScriptDirs);

		auto* scene_menu = new QMenu("&Scripts", parentWidget());
		scene_menu->addAction(action_new_script);
		scene_menu->addAction(action_edit_script);
		scene_menu->addAction(action_add_custom_user_script_dir);
		scene_menu->addAction(action_remove_custom_user_script_dirs);
		scene_menu->addSeparator();
		scene_menu->addAction(action_browse_user_scripts);
		scene_menu->addSeparator();
		scene_menu->addAction(action_refresh_tool_dialogs);
		scene_menu->addSeparator();
		scene_menu->addAction(action_script_editor);

		_menus.push_back(scene_menu);
	}

	if(_scene_inspector){
		if(!_scene_inspector_menu){
			_scene_inspector_menu = new RClickMenu_SceneInspector(_scene_inspector);
			_scene_inspector_menu->setVisible(false);
		}

		connect(_scene_inspector,  &SceneInspector::subsetChanged,
			    _widget_projector, &ProjectorWidget::setActiveSubset);
	}

	if(_scene != scene){
		connect(scene, &LGScene::object_to_be_removed,
			    _widget_projector, &ProjectorWidget::objectToBeRemoved);
		_widget_clip_plane->setScene(scene);
		_scene = scene;
	}
}

void MeshModule::
deactivate()
{
	if(_scene_inspector){
		_scene_inspector->disconnect(_widget_projector);
		_scene_inspector->disconnect(this);
		_scene_inspector = nullptr;
	}

	if(_scene){
		_scene->disconnect(_widget_projector);
		_scene->disconnect(this);
		_scene = nullptr;
	}
}


MeshModule::dock_list_t
MeshModule::
getDockWidgets()
{
	return _dock_widgets;
}

std::vector<QToolBar*>
MeshModule::
getToolBars()
{
	return {};
}

QMenu*
MeshModule::
getSceneInspectorMenu()
{
	return _scene_inspector_menu->getMenu();
}


std::vector<QMenu*>
MeshModule::
getMenus()
{
	return _menus;
}


void MeshModule::
keyPressEvent(QKeyEvent* event)
{
	Qt::KeyboardModifiers qtMods = QApplication::keyboardModifiers();
	uint mods = 0;

	if(qtMods.testFlag(Qt::ShiftModifier))
		mods |= SMK_SHIFT;
	if(qtMods.testFlag(Qt::ControlModifier))
		mods |= SMK_CTRL;
	if(qtMods.testFlag(Qt::AltModifier))
		mods |= SMK_ALT;

	_tool_manager->execute_shortcut(event->key(), mods);
}

void MeshModule::showScriptEditor()
{
	_script_editor->show();
	_script_editor->raise();
	_script_editor->activateWindow();
}

void MeshModule::refreshToolDialogsClicked()
{
	RefreshScriptTools(_tool_manager);
	_tool_browser->refresh(_tool_manager);
}

void MeshModule::browseUserScripts()
{
	//QDir scriptDir = app::UserScriptDir();
	QString path = QDir::toNativeSeparators(app::UserScriptDir().path());
	QDesktopServices::openUrl(QUrl("file:///" + path));
}

void MeshModule::addCustomUserScriptDir()
{
	QString customUserScriptDirPath = QFileDialog::getExistingDirectory(parentWidget(),
													tr("Open Directory"),
													QDir::toNativeSeparators(QDir::home().path()),
	                                                QFileDialog::ShowDirsOnly
	                                                | QFileDialog::DontResolveSymlinks);

	if(!customUserScriptDirPath.isEmpty()){
		QSettings settings;
		QStringList customUserPaths = settings.value("customUserScriptPaths").toStringList();
		customUserPaths.push_back(customUserScriptDirPath);
		settings.setValue("customUserScriptPaths", customUserPaths);
	}

	refreshToolDialogsClicked();
}

void MeshModule::removeCustomUserScriptDirs()
{
	QSettings settings;
	settings.remove("customUserScriptPaths");

	QMessageBox msgBox;
	msgBox.setText("Changes will take effect after restart.");
	msgBox.exec();
}

void MeshModule::newScript()
{
	QString fileName = QFileDialog::getSaveFileName(
									parentWidget(),
									tr("Script Name"),
									QDir::toNativeSeparators(app::UserScriptDir().path()),
									tr("script files (*.lua)"));
	if(!fileName.endsWith(".lua"))
		fileName.append(".lua");

	if(!fileName.isEmpty()){
		if(QFile::exists(fileName))
			QFile::remove(fileName);
		QFile::copy(":/resources/default-script.lua", fileName);
		QFile::setPermissions(fileName, QFileDevice::ReadOwner | QFileDevice::WriteOwner |
										QFileDevice::ReadUser | QFileDevice::WriteUser |
										QFileDevice::ReadGroup);

		QDesktopServices::openUrl(QUrl("file:///" + fileName));
	}
}

void MeshModule::editScript()
{
	QString fileName = QFileDialog::getOpenFileName(
									parentWidget(),
									tr("Script Name"),
									QDir::toNativeSeparators(app::UserScriptDir().path()),
									tr("script files (*.lua)"));

	if(!fileName.isEmpty()){
		QDesktopServices::openUrl(QUrl("file:///" + fileName));
	}
}

void MeshModule::refreshCoordinates()
{
	LGObject* obj = app::getActiveObject();
	vector3 center(0, 0, 0);
	if(obj){
	//	calculate the center of the current selection
		Grid::VertexAttachmentAccessor<APosition> aaPos(obj->grid(), aPosition);
		CalculateCenter(center, obj->selector(), aaPos);
	}

	for(int i = 0; i < 3; ++i)
		_widget_coords->set_value(i, 0, center[i]);
}

void MeshModule::coordinatesChanged()
{
	LGObject* obj = app::getActiveObject();
	if(obj){
		vector3 c(0, 0, 0);
		for(int i = 0; i < 3; ++i)
			c[i] = _widget_coords->value(i, 0);
		promesh::MoveSelectionTo(obj, c);
		obj->write_selection_to_action_log ();
		obj->log_action(QString("MoveSelectionTo (mesh, Vec3d(%1,%2,%3))\n")
								.arg(c[0], 0, 'g', 12)
								.arg(c[1], 0, 'g', 12)
								.arg(c[2], 0, 'g', 12));
		obj->geometry_changed();
	}
}
