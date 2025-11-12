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
#include "scene/scene_interface.hpp"
#include "scene_inspector.hpp"
#include "scene_item_model.hpp"
#include "delegates.hpp"

using namespace std;

////////////////////////////////////////////////////////////////////////
SceneInspector::SceneInspector(QWidget* parent) : QTreeView(parent)
{
//	create objects and connect them
	//m_treeView = new QTreeView(this);

	m_model = new SceneItemModel;
//	if debugging of SceneItemModel is required, then uncomment the following line.
	//new ModelTest(m_model, this);

	this->setModel(m_model);
	this->setItemDelegateForColumn(1, new VisibilityDelegate(1, this));
	this->setItemDelegateForColumn(2, new ColorDelegate(2, SIDR_COLOR_SOLID, this));

	QHeaderView* header = this->header();
	header->setStretchLastSection(false);
	header->setSectionResizeMode(0, QHeaderView::Stretch);
	header->setSectionResizeMode(1, QHeaderView::Fixed);
	header->setSectionResizeMode(2, QHeaderView::Fixed);
	this->resizeColumnToContents(1);
	this->resizeColumnToContents(2);

}

SceneInspector::~SceneInspector()
{
	delete m_model;
}

void SceneInspector::setScene(IScene* scene)
{
	m_model->setScene(scene);
	connect(scene, &IScene::visuals_updated, this, &SceneInspector::refreshView);
}

void SceneInspector::mousePressEvent(QMouseEvent * event)
{
	const int old_active_object_index = getActiveObjectIndex();
	const int old_active_subset_index = getActiveSubsetIndex();

	QModelIndex index = indexAt(event->pos());
	if(index.isValid()){
		if(index.column() == 1)
		{
			bool visible = m_model->data(index, SIDR_VISIBLE).toBool();
			m_model->setData(index, QVariant(!visible), SIDR_VISIBLE);

			if(visible){
				return;
			}
		}
	}

	QTreeView::mousePressEvent(event);
	const int new_active_object_index = getActiveObjectIndex();
	const int new_active_subset_index = getActiveSubsetIndex();

	if(new_active_object_index != old_active_object_index){
		emit objectChanged(getActiveObject());
		emit subsetChanged(getActiveObject(), new_active_subset_index);
	}
	else if(new_active_subset_index != old_active_subset_index)
		emit subsetChanged(getActiveObject(), new_active_subset_index);
}

void SceneInspector::mouseReleaseEvent(QMouseEvent* event)
{
	QTreeView::mouseReleaseEvent(event);
	emit(mouseClicked(event));
}

int SceneInspector::getActiveObjectIndex()
{
	QModelIndex index = currentIndex();
	if(index.isValid()){
		QModelIndex objIndex = m_model->parentObjectIndexFromIndex(index);
		if(objIndex.isValid())
			return objIndex.row();
	}
	return -1;
}

int SceneInspector::getActiveSubsetIndex()
{
	QModelIndex index = currentIndex();
	if(index.isValid()){
		if(index.parent().isValid()){
		//	the index is a subset index
			return index.row();
		}
	}
	return -1;
}

ISceneObject* SceneInspector::getActiveObject()
{
	QModelIndex index = currentIndex();
	if(index.isValid())
		return m_model->objectFromIndex(index);
	return nullptr;
}

void SceneInspector::setActiveObject(int index)
{
	if(index >= 0){
		this->setCurrentIndex(m_model->index(index, 0));
		emit objectChanged(getActiveObject());
		emit subsetChanged(getActiveObject(), getActiveSubsetIndex());
	}
}

void SceneInspector::setActiveSubset(int objIndex, int subsetIndex)
{
	this->setCurrentIndex(m_model->index(subsetIndex, 0, m_model->index(objIndex, 0)));
	emit subsetChanged(getActiveObject(), getActiveSubsetIndex());
}

void SceneInspector::refreshView()
{
	m_model->refreshSubsets();
	auto top_left = QModelIndex();
	auto bottom_right = QModelIndex();
	//if (index.isValid()) {
	//	emit dataChanged(index, index, {Qt::DisplayRole});
	//} else {
		// fallback: refresh everything
	m_model->refresh_all();

	//}

	//dataChanged(top_left, bottom_right);

	emit subsetChanged(getActiveObject(), getActiveSubsetIndex());
}

void SceneInspector::hideSubset(int objIndex, int subsetIndex)
{
	QModelIndex objModelIndex = m_model->index(objIndex, 1, QModelIndex());
	QModelIndex index = m_model->index(subsetIndex, 1, objModelIndex);
	if(index.isValid())
		m_model->setData(index, QVariant(false), SIDR_VISIBLE);
}

void SceneInspector::showSubset(int objIndex, int subsetIndex)
{
	QModelIndex objModelIndex = m_model->index(objIndex, 1, QModelIndex());
	QModelIndex index = m_model->index(subsetIndex, 1, objModelIndex);
	if(index.isValid())
		m_model->setData(index, QVariant(true), SIDR_VISIBLE);
}
