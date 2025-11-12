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

#ifndef __H__PROMESH_mesh_module
#define __H__PROMESH_mesh_module

#include "module_interface.hpp"

class RClickMenu_SceneInspector;
class ProjectorWidget;
class ToolManager;
class ToolBrowser;
class ProjectorWidget;
class ClipPlaneWidget;
class MatrixWidget;
class QScriptEditor;


class MeshModule : public IModule {
	Q_OBJECT
public:
	MeshModule () = default;

	explicit MeshModule (QWidget* parent);

	~MeshModule () override = default;

	void activate(SceneInspector* sceneInspector, LGScene* scene) override;

	void deactivate() override;

	dock_list_t
	getDockWidgets() override;

	std::vector<QToolBar*>
	getToolBars() override;

	QMenu* getSceneInspectorMenu() override;

	std::vector<QMenu*> getMenus() override;

	void keyPressEvent(QKeyEvent* event) override;

	
protected slots:
	void refreshToolDialogsClicked();
	void browseUserScripts();
	void addCustomUserScriptDir();
	void removeCustomUserScriptDirs();
	void newScript();
	void editScript();
	void refreshCoordinates();
	void coordinatesChanged();
	void showScriptEditor();

private:
	dock_list_t _dock_widgets;
	std::vector<QMenu*>	_menus;

	SceneInspector* _scene_inspector;
	LGScene* _scene;

	RClickMenu_SceneInspector* _scene_inspector_menu;

	ToolManager* _tool_manager;
	ToolBrowser* _tool_browser;

	ClipPlaneWidget* _widget_clip_plane;
	ProjectorWidget* _widget_projector;
	MatrixWidget* _widget_coords;
	QScriptEditor* _script_editor;


};
#endif