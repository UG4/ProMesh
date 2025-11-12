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

#ifndef __H__MAIN_WINDOW__
#define __H__MAIN_WINDOW__

#include <QMainWindow>
#include <QModelIndex>
#include <QSettings>
#include "color_widget.hpp"
#include "scene/lg_object.hpp"
#include "view3d/view3d.hpp"
#include "scene/lg_scene.hpp"
#include "scene_inspector.hpp"
#include "modules/module_interface.hpp"

////////////////////////////////////////////////////////////////////////
//	predeclarations
class View3D;
class LGScene;
class ISceneObject;

class QAction;
class QComboBox;
class QFileDialog;
class QHelpBrowser;
class QPushButton;
class QPoint;
class QPlainTextEdit;
class QTreeView;
class QToolBar;
class QToolButton;
class PropertyWidget;
class SceneInspector;
class ToolManager;
class ToolBrowser;
class QScriptEditor;
class TruncatedDoubleSpinBox;


enum SceneObjectType {
	SOT_LG,
	SOT_CSG
};

////////////////////////////////////////////////////////////////////////
///	the main window.
/**
 * This class organizes the toolbars, menus, views and tool-windows
 * of the application.
 */
class MainWindow : public QMainWindow {
	Q_OBJECT

	protected:
		enum MouseMoveAction{
			MMA_DEFAULT = TT_NONE,
			MMA_GRAB = TT_GRAB,
			MMA_ROTATE = TT_ROTATE,
			MMA_SCALE = TT_SCALE
		};

		enum Axis{
			X_AXIS = 1,
			Y_AXIS = 1<<1,
			Z_AXIS = 1<<2
		};

	public:
		MainWindow();
		~MainWindow() override = default;

		void init();

		LGScene* get_scene() {return _scene;}

		bool load_grid_from_file(const char* filename);
		bool save_object_to_file(ISceneObject* obj, const char* filename);
        LGObject* create_empty_object(const char* name, SceneObjectType sot);
		QSettings& settings() {return _settings;}

		LGObject* getActiveObject();
		View3D*	getView3D() {return _p_view;}
		SceneInspector* getSceneInspector() {return _scene_inspector;}

		void launchHelpBrowser(const QString& pageName);
		
		void check_options() const;
		
		const char* log_text();
		
	signals:
		void activeObjectChanged();
		void refreshToolDialogs();

	public slots:
		void setActiveObject(int index);
		void newGeometry();
		int openFile();///< returns the number of successfully opened files.
		int loadIntoMesh();
		bool reloadActiveGeometry();
		bool reloadAllGeometries();
		bool saveToFile();
		bool exportToUG3();
		void eraseActiveSceneObject();///< erases the active scene object.
		void showHelp();
		void showScriptReference();
		void showRecentChanges();
		void showControls();
		void showShortcuts();
		void showLicense();
		void showAbout();
		void showContact();
		void quit();
		void saveOptions();
		void loadOptions();
		void refreshOptions();

	protected slots:
		void frontDrawModeChanged(int newMode);
		void backDrawModeChanged(int newMode);
		void backgroundColorChanged(const QColor& color);
		void view3dMousePressed(QMouseEvent *event);
		void view3dMouseMoved(QMouseEvent *event);
		void view3dMouseReleased(QMouseEvent *event);
		void view3dKeyReleased(QKeyEvent* event);
		void selectionElementChanged(int newElement);
		void selectionElementChanged(bool enabled);
		void selectionModeChanged(int newMode);
		void elementDrawModeChanged();
		void undo();
		void redo();
		void sceneInspectorClicked(QMouseEvent* event);
		void optionsChanged();
		void refreshActionLog(ISceneObject* obj);
		void actionLogChanged(const QString& newContent);
		void actionLogCleared();
		void viewScaleXChanged(double value);
		void viewScaleYChanged(double value);
		void viewScaleZChanged(double value);

	protected:
		void closeEvent(QCloseEvent *event) override;

		void dragEnterEvent(QDragEnterEvent* event) override;
		void dropEvent(QDropEvent* event) override;

		void mousePressEvent(QMouseEvent* event) override;
		void mouseMoveEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;
		void keyPressEvent(QKeyEvent* event) override;

		QToolBar* createVisibilityToolbar();

		template <typename TElem>
		void selectElement(LGObject* obj, TElem* elem, bool extendSelection);

		uint getLGElementMode();

		void beginMouseMoveAction(MouseMoveAction mma);
		void updateMouseMoveAction();
		void endMouseMoveAction(bool bApply);

	///	casts a ray to the camera-focus plane and places the vertex there
	/**	The method can be fine-tuned through options.drawPath.*/
		void insertVertexAtScreenCoord(number x, number y);

		void populateMenuBar();
		void activateModule(IModule* mod);
		
	protected:
	//	3d view
		View3D* _p_view;

		LGScene* _scene;

	//	Modules
		IModule* _active_module;
		IModule::dock_list_t _module_dock_widgets;
		std::vector<QMenu*> _module_menus;

	//	tools
		ColorWidget* _bg_color;
		QSettings _settings;

	//	state-variables
		int _selection_element;
		int	_selection_mode;
		int _cur_selection_mode; ///< stores the selection mode of the current click. Does not necessarily equal m_selectionMode.
		int _element_mode_list_index;
		int _mouse_move_action;

	//	important for selection etc
		QPoint _mouse_down_pos;
		QPoint _mouse_move_action_start;
		LGObject* _mouse_move_action_object;///< Only valid if m_mouseMoveAction != MMA_DEFAULT
		unsigned int _active_axis;

	//	use it as seldom as possible. It is mainly used to trigger the signal activeObjectChanged.
		LGObject* _active_object;
		LGObject* _action_log_sender;

	//	dialogs
		QFileDialog* _dlg_geometry_files;
		SceneInspector* _scene_inspector;
		QDialog* _dlg_about;
		PropertyWidget* _opt_widget;
		QDockWidget* _p_log;
		QPlainTextEdit* _p_log_text;
		QPlainTextEdit* _action_log;
		QMenu* _scene_inspector_r_click_menu;
		TruncatedDoubleSpinBox* _view_scale_x;
		TruncatedDoubleSpinBox* _view_scale_y;
		TruncatedDoubleSpinBox* _view_scale_z;

		#ifdef PROMESH_USE_WEBKIT
			QHelpBrowser*			_help_browser;
		#endif

	//	menus
		QMenu* _menu_file;
		QMenu* _menu_help;

	//	actions
		QAction* _action_new;
		QAction* _action_open;
		QAction* _action_loac_into_mesh;
		QAction* _action_save;
		QAction* _action_reload;
		QAction* _action_reload_all;
		QAction* _action_export;
		QAction* _action_erase;
		QAction* _action_export_ug3;
		QAction* _action_quit;

		QAction* _action_help;
		QAction* _action_recent_changes;
		QAction* _action_controls;
		QAction* _action_shortcuts;
		QAction* _action_license;
		QAction* _action_jump_to_script_reference;
		QAction* _action_show_about;
		QAction* _action_show_contact;

		QToolButton* _tb_sel_vrts;
		QToolButton* _tb_sel_edges;
		QToolButton* _tb_sel_faces;
		QToolButton* _tb_sel_vols;

		QToolButton* _tb_render_vrts;
		QToolButton* _tb_render_edges;
		QToolButton* _tb_render_faces;
		QToolButton* _tb_render_vols;

		QComboBox* _sel_modes;
};

#endif