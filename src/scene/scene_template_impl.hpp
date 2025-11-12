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

#ifndef __H__SCENE_TEMPLATE_IMPL__
#define __H__SCENE_TEMPLATE_IMPL__

#include <vector>
#include "scene_template.hpp"

template <typename TObject>
TScene<TObject>::
TScene() : IScene()
{
}

template <typename TObject>
TScene<TObject>::
~TScene()
{
	for(int i = 0; i < num_objects(); ++i)
	{
		if(_v_infos[i]._auto_delete)
			delete _v_objects[i];
	}
}

////////////////////////////////////////////////////////////////////////
//	new methods
template <typename TObject>
int TScene<TObject>::
add_object(TObject* obj, bool autoDelete)
{
//	make sure, that the object is not already contained.
	int oldInd = get_object_index(obj);
	assert(oldInd == -1 && "object is already contained in the scene!");

	if(oldInd == -1)
	{
		int newInd = num_objects();
		_v_objects.push_back(obj);
		_v_infos.push_back(ObjectInfo(autoDelete));
		emit IScene::object_added(obj);
		return newInd;
	}
	return -1;
}

template <typename TObject>
TObject* TScene<TObject>::
get_object(int index)
{
	if(index_is_valid(index))
		return _v_objects[index];
	return nullptr;
}

////////////////////////////////////////////////////////////////////////
//	derived from IScene
template <typename TObject>
bool TScene<TObject>::
erase_object(int index)
{
	if(index_is_valid(index))
	{
		TObject* obj = get_object(index);
		bool retVal = remove_object(index);

		if(_v_infos[index]._auto_delete)
		{
			emit IScene::object_to_be_erased(obj);
			delete obj;
		}

		return retVal;
	}

	return false;
}

template <typename TObject>
bool TScene<TObject>::
remove_object(int index)
{
	if(index_is_valid(index))
	{
		ISceneObject* obj = get_object(index);
		emit object_to_be_removed(obj);
		_v_objects.erase(_v_objects.begin() + index);
		_v_infos.erase(_v_infos.begin() + index);
		emit visuals_updated();
		emit object_removed();
		return true;
	}

	return false;
}

template <typename TObject>
int TScene<TObject>::
num_objects() const
{
	return (int)_v_objects.size();
}

template <typename TObject>
ISceneObject* TScene<TObject>::
get_scene_object(int index)
{
	if(index_is_valid(index))
		return _v_objects[index];
	return nullptr;
}

template <typename TObject>
int TScene<TObject>::
get_object_index(ISceneObject* pObj)
{
	for(size_t i = 0; i < _v_objects.size(); ++i)
	{
		if(_v_objects[i] == pObj)
			return (int)i;
	}
	return -1;
}

template <typename TObject>
void TScene<TObject>::
update_visuals(int objIndex)
{
//TODO: emit signal
}

template <typename TObject>
void TScene<TObject>::
update_visuals(ISceneObject* pObj)
{
//TODO: emit signal
}

template <typename TObject>
void TScene<TObject>::
object_changed(int objIndex)
{
//TODO: emit signal
}

template <typename TObject>
void TScene<TObject>::
object_changed(ISceneObject* pObj)
{
//TODO: emit signal
}

#endif
