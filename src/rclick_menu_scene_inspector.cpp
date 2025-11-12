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

#include <QtWidgets>
#include "rclick_menu_scene_inspector.hpp"
#include "app.hpp"
#include "tools/tool_dialog.hpp"
#include "tools/subset_tools.h"
#include "tools/selection_tools.h"

using namespace std;
using namespace ug;

RClickMenu_SceneInspector::
RClickMenu_SceneInspector(SceneInspector * scene_inspector) :
			QWidget(scene_inspector), _scene_inspector(scene_inspector)
{
	_menu = new QMenu(this);

//	populate the menu
	_action_assign_subset = new QAction(tr("Assign To Subset"), this);
	connect(_action_assign_subset, &QAction::triggered, this, &RClickMenu_SceneInspector::assignSubset);
	_menu->addAction(_action_assign_subset);

	_action_assign_new_subset = new QAction(tr("Assign To New Subset"), this);
	connect(_action_assign_new_subset, &QAction::triggered, this, &RClickMenu_SceneInspector::assignNewSubset);
	_menu->addAction(_action_assign_new_subset);

	_action_rename = new QAction(tr("Rename"), this);
	connect(_action_rename, &QAction::triggered, this, &RClickMenu_SceneInspector::rename);
	_menu->addAction(_action_rename);

	_action_select_subset = new QAction(tr("Select Subset"), this);
	connect(_action_select_subset, &QAction::triggered, this, &RClickMenu_SceneInspector::selectSubset);
	_menu->addAction(_action_select_subset);
	
	_action_show_all_subsets = new QAction(tr("Show All Subsets"), this);
	connect(_action_show_all_subsets, &QAction::triggered, this, &RClickMenu_SceneInspector::showAllSubsets);
	_menu->addAction(_action_show_all_subsets);

	_action_hide_all_subsets = new QAction(tr("Hide All Subsets"), this);
	connect(_action_hide_all_subsets, &QAction::triggered, this, &RClickMenu_SceneInspector::hideAllSubsets);
	_menu->addAction(_action_hide_all_subsets);

	_action_toggle_all_subset_visibilities = new QAction(tr("Toggle All Subset Visibilities"), this);
	connect(_action_toggle_all_subset_visibilities, &QAction::triggered, this, &RClickMenu_SceneInspector::toggleAllSubsetVisibilities);
	_menu->addAction(_action_toggle_all_subset_visibilities);

	_action_print_subset_contents = new QAction(tr("Print Subset Contents"), this);
	connect(_action_print_subset_contents, &QAction::triggered, this, &RClickMenu_SceneInspector::printSubsetContents);
	_menu->addAction(_action_print_subset_contents);

	_action_reload = new QAction(tr("Reload"), this);
	connect(_action_reload, &QAction::triggered, this, &RClickMenu_SceneInspector::reload);
	_menu->addAction(_action_reload);

}

void RClickMenu_SceneInspector::
exec(const QPoint& p){
	_menu->exec(p);
}

void RClickMenu_SceneInspector::assignSubset()
{
	LGObject* obj = app::getActiveObject();
	if(obj){
		int si = _scene_inspector->getActiveSubsetIndex();
		if(si != -1){
			obj->write_selection_to_action_log();
			obj->log_action (QString("AssignSubset (mesh, %1, true, true, true, true)\n").
								arg(si));
			promesh::AssignSubset(obj, si, true, true, true, true);
			obj->geometry_changed();
		}
	}
}

void RClickMenu_SceneInspector::assignNewSubset()
{
	LGObject* obj = app::getActiveObject();
	if(obj){
		int si = obj->subset_handler().num_subsets();
		obj->write_selection_to_action_log();
		obj->log_action (QString("AssignSubset (mesh, %1, true, true, true, true)\n").
								arg(si));
		promesh::AssignSubset(obj, si, true, true, true, true);

		obj->geometry_changed();

		int activeObjectIndex = _scene_inspector->getActiveObjectIndex();
		_scene_inspector->setActiveSubset(activeObjectIndex, si);
		rename();
	}
}

void RClickMenu_SceneInspector::rename()
{
//	create a tool dialog with a single text-input box.
	this->close();
	LGObject* obj = app::getActiveObject();
	if(obj){
		/*ToolWidget* widget = new ToolWidget("rename", this,
										  nullptr, IDB_APPLY);

		QDialog* dlg = new QDialog(nullptr);
		QVBoxLayout* layout = new QVBoxLayout(dlg);
		dlg->setLayout(layout);
		layout->addWidget(widget);

		string curName = obj->name();
		int si = _scene_inspector->getActiveSubsetIndex();
		if(si != -1)
			curName = obj->get_subset_name(si);
		
		widget->addTextBox("name:", curName.c_str());
		*/
		string curName = obj->name();
		int si = _scene_inspector->getActiveSubsetIndex();
		if(si != -1)
			curName = obj->get_subset_name(si);

		auto* dlg = new QDialog(this);
		dlg->setWindowTitle(tr("rename"));
		auto* layout = new QVBoxLayout(dlg);
		dlg->setLayout(layout);

		auto* text = new QLineEdit(dlg);
		layout->addWidget(text);
		text->setText(QString::fromUtf8(curName.c_str()));
		text->selectAll();

		auto* hlayout = new QHBoxLayout();
		layout->addLayout(hlayout);

		hlayout->addStretch();

		auto* btnCancel = new QPushButton(dlg);
		btnCancel->setText(tr("Cancel"));
		hlayout->addWidget(btnCancel);
		connect(btnCancel, &QPushButton::clicked, dlg, &QDialog::reject);

		auto* btnOk = new QPushButton(dlg);
		btnOk->setText(tr("Ok"));
		hlayout->addWidget(btnOk);
		btnOk->setDefault(true);
		connect(btnOk, &QPushButton::clicked, dlg, &QDialog::accept);

		if(dlg->exec()){
			curName = text->text().toLocal8Bit().constData();
			if(si != -1){
				obj->set_subset_name(si, curName.c_str());
				obj->log_action (QString("SetSubsetName (mesh, %1, \"%2\")\n").
									arg(si).arg(curName.c_str()));
			}
			else
				obj->set_name(curName.c_str());
			obj->set_save_required(true);
			_scene_inspector->refreshView();
		}
		delete dlg;
	}
}

void RClickMenu_SceneInspector::selectSubset()
{
	LGObject* obj = app::getActiveObject();
	if(obj){
		int si = _scene_inspector->getActiveSubsetIndex();
		if(si != -1){
			obj->write_selection_to_action_log();
			obj->log_action (QString("SelectSubset (mesh, %1, true, true, true, true)\n").
								arg(si));
			promesh::SelectSubset(obj, si, true, true, true, true);
			obj->selection_changed();
		}
	}
}

void RClickMenu_SceneInspector::
showAllSubsets(){
	ISceneObject* obj = _scene_inspector->getActiveObject();
	if(obj){
		for(int i = 0; i < obj->num_subsets(); ++i){
			obj->set_subset_visibility(i, true);
		}
		obj->visuals_changed();
		_scene_inspector->refreshView();
	}
}

void RClickMenu_SceneInspector::
printSubsetContents()
{
	auto* obj = dynamic_cast<LGObject*>(_scene_inspector->getActiveObject());
	int si = _scene_inspector->getActiveSubsetIndex();
	if(obj && (si != -1)){
		PrintElementNumbers(obj->subset_handler().get_grid_objects_in_subset(si));
	}
}

void RClickMenu_SceneInspector::
hideAllSubsets(){
	ISceneObject* obj = _scene_inspector->getActiveObject();
	if(obj){
		for(int i = 0; i < obj->num_subsets(); ++i){
			obj->set_subset_visibility(i, false);
		}
		obj->visuals_changed();
		_scene_inspector->refreshView();
	}
}

void RClickMenu_SceneInspector::
toggleAllSubsetVisibilities(){
	ISceneObject* obj = _scene_inspector->getActiveObject();
	if(obj){
		for(int i = 0; i < obj->num_subsets(); ++i){
			obj->set_subset_visibility(i, !obj->subset_is_visible(i));
		}
		obj->visuals_changed();
		_scene_inspector->refreshView();
	}
}

void RClickMenu_SceneInspector::
reload(){
	this->close();
	LGObject* obj = app::getActiveObject();
	if(obj){
		ReloadLGObject(obj);
	}
}