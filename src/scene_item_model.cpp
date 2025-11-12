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
#include "scene_item_model.hpp"

using namespace std;

SceneItemModel::SceneItemModel()
{
	_scene = nullptr;
	_icon_visible.addFile(":images/visible_16.png");
	_icon_invisible.addFile(":images/invisible_16.png");
	_icon_color.addFile(":images/cube_solid.png");
}


void SceneItemModel::setScene(IScene* scene)
{
//	notify view
	emit layoutAboutToBeChanged();

//	update connections
	if(_scene)
	{
		disconnect(_scene, &IScene::object_added, this, &SceneItemModel::newObject);
		disconnect(_scene, &IScene::object_to_be_removed, this, &SceneItemModel::removeObject);
	}

	_scene = scene;

	if(_scene)
	{
		connect(_scene, &IScene::object_added, this, &SceneItemModel::newObject);
		connect(_scene, &IScene::object_to_be_removed, this, &SceneItemModel::removeObject);
	}

//	notify view
	emit layoutChanged();
}

void SceneItemModel::refreshSubsets()
{
	if(!_scene)
		return;

//	first we'll clear the subset-entries of all objects
	for(size_t i = 0; i < _item_infos.size(); ++i){
	//	the model-index of the i-th scene object
		QModelIndex objModelIndex = index((int)i, 0, QModelIndex());
		SceneItemInfo* psii = _item_infos[i];
		ISceneObject* obj = psii->_obj;

		if((int)psii->_children.size() > obj->num_subsets()){
		//	notify that subsets are removed from the list
			beginRemoveRows(objModelIndex, obj->num_subsets(),
							(int)psii->_children.size()-1);

		//	call update to adjust children
			updateItemInfo(psii);

		//	removal done. inform the model.
			endRemoveRows();
		}
		else if((int)psii->_children.size() < obj->num_subsets()){
		//	notify base class
			beginInsertRows(objModelIndex, (int)psii->_children.size(),
							obj->num_subsets()-1);

		//	call update to adjust children
			updateItemInfo(psii);

		//	notify base class
			endInsertRows();
		}
		else{
		//	call update to adjust children
			updateItemInfo(psii);
		}
	}
}

Qt::ItemFlags SceneItemModel::flags ( const QModelIndex & index ) const
{
	int col = index.column();
	switch(col)
	{
		case 0: return Qt::ItemIsSelectable | Qt::ItemIsEnabled/* | Qt::ItemIsEditable*/;
		case 1: return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
		case 2: return Qt::ItemIsEnabled | Qt::ItemIsSelectable| Qt::ItemIsEditable;
	}
	return Qt::NoItemFlags;
}

QVariant SceneItemModel::headerData ( int section,
							Qt::Orientation orientation,
							int role) const
{
	switch(section)
	{
		case 0:
		{
			if(role == Qt::DisplayRole)
				return QString(tr("geometry name"));
		}break;
		case 1:
		{
			if(role == Qt::DecorationRole)
				return _icon_visible;
		}break;
		case 2:
		{
			if(role == Qt::DecorationRole)
				return _icon_color;
		}break;
	}

	return {};
}

int SceneItemModel::rowCount ( const QModelIndex & parent) const
{
	if(_scene)
	{
		SceneItemInfo* itemInfo = itemInfoFromIndex(parent);
		if(itemInfo)
			return (int)itemInfo->_children.size();
		else
			return (int)_item_infos.size();
	}

	return 0;
}

int SceneItemModel::columnCount ( const QModelIndex & parent) const
{
	// if(parent.isValid()) return 3;
	return 3;
}

QVariant SceneItemModel::data ( const QModelIndex & index,
								int role) const
{

	int col = index.column();

	if(SceneItemInfo* itemInfo = itemInfoFromIndex(index))
	{
		switch(col)
		{
			case 0:
			{
				if(role == Qt::DisplayRole)
				{
					switch(itemInfo->_type)
					{
						case SIT_OBJECT:
							return QString::fromUtf8(itemInfo->_obj->name());
						case SIT_SUBSET:{
							QString name = QString::number(itemInfo->_index);
							name.append(": ");
							name.append(QString::fromUtf8(itemInfo->_obj->get_subset_name(itemInfo->_index)));
							return name;
						}
					}
					return QString(tr("UNKNOWN NAME"));
				}
			}break;
			case 1:
			{
				if(role == SIDR_VISIBLE)
				{
					if(itemInfo->_type == SIT_OBJECT)
						return QVariant(itemInfo->_obj->is_visible());
					else if(itemInfo->_type == SIT_SUBSET)
						return QVariant( itemInfo->_obj->subset_is_visible(itemInfo->_index));
					return QVariant(true);
				}
			}break;
			case 2:
			{
				if(role == SIDR_COLOR_SOLID)
				{
					if(itemInfo->_type == SIT_OBJECT)
						return QVariant((uint)itemInfo->_obj->get_color().rgb());
					else if(itemInfo->_type == SIT_SUBSET)
						return QVariant( (uint)itemInfo->_obj->get_subset_color( itemInfo->_index).rgb());
					return QVariant((uint)Qt::red);
				}
			}break;
		}
	}

	return QVariant();
}

bool SceneItemModel::setData ( const QModelIndex & index,
								const QVariant & value,
								int role)
{
	if(SceneItemInfo* itemInfo = itemInfoFromIndex(index))
	{
		if(index.column() == 0 && role == Qt::EditRole)
		{
			if(value.toString().isEmpty())
				return false;

			if(itemInfo->_type == SIT_OBJECT)
				itemInfo->_obj->set_name(value.toString().toLatin1());
			else if(itemInfo->_type == SIT_SUBSET)
				itemInfo->_obj->set_subset_name(itemInfo->_index,
									value.toString().toLatin1());
			else
				return false;

			emit dataChanged(index, index);
			return true;
		}

		if(index.column() == 1 && role == SIDR_VISIBLE)
		{
			{
				bool updateVisuals = true;
				if(itemInfo->_type == SIT_OBJECT)
					itemInfo->_obj->set_visibility(value.toBool());
				else if(itemInfo->_type == SIT_SUBSET)
					itemInfo->_obj->set_subset_visibility(
							itemInfo->_index, value.toBool());
				else
					updateVisuals = false;

			//	the visuals have to be updated
				if(updateVisuals)
				{
					itemInfo->_obj->visuals_changed(false);
					//m_scene->visibility_changed(itemInfo->_obj);
				}

			}
			emit dataChanged(index, index);
			return true;
		}

		if(index.column() == 2 && role == SIDR_COLOR_SOLID)
		{
		//	extract color from QVariant
			bool ok = false;
			QRgb col = (QRgb)value.toUInt(&ok);
			if(ok)
			{
				bool updateVisuals = true;
				if(itemInfo->_type == SIT_OBJECT)
					itemInfo->_obj->set_color(QColor(col));
				else if(itemInfo->_type == SIT_SUBSET)
					itemInfo->_obj->set_subset_color(
									itemInfo->_index, QColor(col));
				else
					updateVisuals = false;

			//	the visuals have to be updated
				if(updateVisuals)
				{
					_scene->color_changed(itemInfo->_obj);
				}
				//std::cout << "A--- " << index << std::endl;
				emit dataChanged(index, index);
				return true;
			}
		}
	}

	return QAbstractItemModel::setData(index, value, role);
}

QModelIndex SceneItemModel::index ( int row, int column,
									const QModelIndex & parent) const
{
	if(row < 0 || column < 0)
		return QModelIndex();

	if(_scene)
	{/*
		if(parent.isValid()){
			if(parent.column() == 1)
				return QModelIndex();
		}*/

		SceneItemInfo* parentInfo = itemInfoFromIndex(parent);
		if(!parentInfo)
		{
		//	since the parent is invalid, the index is a top-level index.
		//	ItemInfos for top-level objects are stored in m_itemInfos.
			if(row < (int)_item_infos.size())
				return createIndex(row, column, _item_infos[row]);
		}
		else
		{
		//	make sure, that the given row is ok
			if(row < (int)parentInfo->_children.size())
			{
				return createIndex(row, column, parentInfo->_children[row]);//	subsets are handled by indices.
			}
		}
	}
	return QModelIndex();
}

QModelIndex SceneItemModel::parent ( const QModelIndex & index ) const
{
	if(index.isValid())
	{
	//	return item-index of parent
		if(SceneItemInfo* itemInfo = itemInfoFromIndex(index))
		{
			if(itemInfo->_parent)
				return indexFromItemInfo(itemInfo->_parent, 0);
		}
	}
	return QModelIndex();
}

void SceneItemModel::newObject(ISceneObject* obj)
{
	if(!_scene)
		return;

//	row-index of the new object:
	int rowIndex = _scene->num_objects() - 1;

//	notify base class
	beginInsertRows(QModelIndex(), rowIndex, rowIndex);

//	create a new ItemInfo.
	auto* itemInfo = new SceneItemInfo;
	_item_infos.push_back(itemInfo);
	itemInfo->_type = SIT_OBJECT;
	itemInfo->_obj = obj;
	itemInfo->_parent = nullptr;

//	call update to populate children
	updateItemInfo(itemInfo);

//	notify base class
	endInsertRows();
}

void SceneItemModel::removeObject(ISceneObject* obj)
{
	if(!_scene)
		return;

//	row index of obj
	int row = _scene->get_object_index(obj);
	if(row < 0)
		return;

//	inform the model about item removal
	beginRemoveRows(QModelIndex(), row, row);

//	erase the item-info entry
	eraseItemInfo(row);

//	removal done. inform the model.
	endRemoveRows();
}

SceneItemInfo* SceneItemModel::itemInfoFromIndex(const QModelIndex& index) const
{
	if(index.isValid())
		return static_cast<SceneItemInfo*>(index.internalPointer());
	return nullptr;
}

ISceneObject* SceneItemModel::objectFromIndex(const QModelIndex& index) const
{
	if(SceneItemInfo* itemInfo = itemInfoFromIndex(index))
		return itemInfo->_obj;
	return nullptr;
}

QModelIndex SceneItemModel::parentObjectIndexFromIndex(const QModelIndex& index) const
{
	QModelIndex tInd = index;
	while(tInd.isValid()){
		SceneItemInfo* info = itemInfoFromIndex(tInd);
		if(info->_type == SIT_OBJECT)
			return tInd;
		tInd = tInd.parent();
	}
	return QModelIndex();
}

QModelIndex SceneItemModel::indexFromItemInfo(SceneItemInfo* itemInfo,
											  int column) const
{
	SceneItemInfo* parentInfo = itemInfo->_parent;

	if(parentInfo)
	{
	//	get the index at which the item is stored by searching
	//	the parents children list
		for(size_t i = 0; i < parentInfo->_children.size(); ++i)
		{
			if(parentInfo->_children[i] == itemInfo)
				return createIndex((int)i, column, itemInfo);
		}
	}
	else
	{
	//	get the index at which the item is stored by searching
	//	the the classes m_itemInfo.
		for(size_t i = 0; i < _item_infos.size(); ++i)
		{
			if(_item_infos[i] == itemInfo)
				return createIndex((int)i, column, itemInfo);
		}
	}

	return QModelIndex();
}

void SceneItemModel::updateItemInfo(SceneItemInfo* itemInfo)
{
	switch(itemInfo->_type)
	{
		case SIT_OBJECT:
		{
		//	clear children
			if(itemInfo->_obj->num_subsets() < (int)itemInfo->_children.size())
			{
				for(size_t i = itemInfo->_obj->num_subsets();
					i < itemInfo->_children.size(); ++i){
					delete itemInfo->_children[i];
				}
				itemInfo->_children.resize(itemInfo->_obj->num_subsets());
			}

		//	add children
			for(int i = (int)itemInfo->_children.size();
				i < itemInfo->_obj->num_subsets(); ++i)
			{
				auto* newInfo = new SceneItemInfo;
				newInfo->_obj = itemInfo->_obj;
				newInfo->_type = SIT_SUBSET;
				newInfo->_index = i;
				newInfo->_parent = itemInfo;
				itemInfo->_children.push_back(newInfo);
			}
		}break;
	}
}

void SceneItemModel::eraseItemInfo(int index)
{
//	remove the entry from m_itemInfos
	SceneItemInfo* psi = *(_item_infos.begin() + index);
	for(size_t i = 0; i < psi->_children.size(); ++i)
		delete psi->_children[i];

	delete *(_item_infos.begin() + index);

	_item_infos.erase(_item_infos.begin() + index);
}

