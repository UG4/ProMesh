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

#ifndef __H__SCENE_TEMPLATE__
#define __H__SCENE_TEMPLATE__

#include <vector>
#include "scene_interface.hpp"

////////////////////////////////////////////////////////////////////////
///	Provides a basic implementation of the scene interface.
/**
 * Implements object handling and slots.
 * Template parameter TObject has to derive from ISceneObject.
 */

template <typename TObject>
class TScene : public IScene {
	public:
		TScene();

		~TScene() override;

	//	new methods
		virtual int add_object(TObject* obj, bool autoDelete = true);
		virtual TObject* get_object(int index);
		//virtual TObject* get_object_by_name(const char* name);

	//	derived from IScene
		bool erase_object(int index) override;

		bool remove_object(int index) override;

		int num_objects() const override;

		ISceneObject* get_scene_object(int index) override;
		//virtual ISceneObject* get_scene_object_by_name(const char* name);
		int get_object_index(ISceneObject* pObj) override;

		void update_visuals(int objIndex) override;

		void update_visuals(ISceneObject* pObj) override;

		void object_changed(int objIndex) override;

		void object_changed(ISceneObject* pObj) override;

	protected:
		bool index_is_valid(int index) {return index >= 0 && index < (int)_v_objects.size();}

	protected:
		struct ObjectInfo
		{
			ObjectInfo() = default;
			explicit ObjectInfo(bool autoDelete) : _auto_delete(autoDelete)	{}
			bool _auto_delete;
		};

		using ObjectVec = std::vector<TObject*>;
		using InfoVec = std::vector<ObjectInfo>;

	protected:
		ObjectVec _v_objects;
		InfoVec _v_infos;
};


////////////////////////////////////////////////
//	include implementation
#include "scene_template_impl.hpp"

#endif
