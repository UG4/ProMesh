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

#ifndef __H__LG_OBJECT__
#define __H__LG_OBJECT__

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

#include <QOpenGLWidget>

#include <QObject>
#include "scene_interface.hpp"
#include "lg_include.hpp"
#include "mesh.h"
#include "undo.hpp"

////////////////////////////////////////////////////////////////////////
//	predeclarations
class LGObject;

////////////////////////////////////////////////////////////////////////
//	constants
///	constants to store in the subset-states.
enum LGSubsetState
{
///	marks a subset as initialized
	LGSS_INITIALIZED = ug::SS_USER_STATE << 1,
///	marks a subset as visible
	LGSS_VISIBLE = ug::SS_USER_STATE << 2
};

///	used to set the element-type that is displayed.
enum LGElementMode
{
	LGEM_NONE = 0,
	LGEM_VERTEX = 1,
	LGEM_EDGE = 1<<1 | LGEM_VERTEX,
	LGEM_FACE = 1<<2 | LGEM_EDGE,
	LGEM_VOLUME = 1<<3 | LGEM_FACE
};

enum LGRenderMode
{
	LGRM_DOUBLE_PASS_SHADED,
	LGRM_SINGLE_PASS_COLOR,
	LGRM_DOUBLE_PASS_COLOR,
	LGRM_SINGLE_PASS_NO_LIGHT
};

enum TransformType{
	TT_NONE,
	TT_GRAB,
	TT_ROTATE,
	TT_SCALE
};

////////////////////////////////////////////////////////////////////////
//	global constants
extern const char* LG_SUPPORTED_FILE_FORMATS_OPEN;
extern const char* LG_SUPPORTED_FILE_FORMATS_SAVE;

////////////////////////////////////////////////////////////////////////
//	methods
LGObject* CreateLGObjectFromFile(const char* filename);
LGObject* CreateEmptyLGObject(const char* name);
bool LoadLGObjectFromFile(LGObject* pObjOut, const char* filename, bool performLoadPostprocessing = true);
void PerformLoadPostprocessing(LGObject* obj);
bool SaveLGObjectToFile(LGObject* pObj, const char* filename);
bool ReloadLGObject(LGObject* obj);

////////////////////////////////////////////////////////////////////////
//	LGObject
///	holds a grid, a subset-handler and the render-object.
class LGObject : public ISceneObject, public ug::promesh::Mesh {
	Q_OBJECT

	public:
		LGObject();
		explicit LGObject(const char* name);

		~LGObject() override = default;

	//	from ISceneObject
		const char* name() override {return _name.c_str();}
		void set_name(const char* name) override {_name = name; properties_changed();}

		void set_visibility(bool visible) override {_visible = visible;}
		bool is_visible() override {return _visible;}

		QColor get_color() const override {return _color;}
		void set_color(const QColor& color) override {_color = color;}

		int num_subsets() override {return m_subsetHandler.num_subsets();}
		const char* get_subset_name(int index) const override {return m_subsetHandler.subset_info(index).name.c_str();}
		void set_subset_name(int index, const char* name) override {m_subsetHandler.subset_info(index).name = name; properties_changed();}

		void set_subset_visibility(int index, bool visible) override;
		bool subset_is_visible(int index) override;
		QColor get_subset_color(int index) const override;
		void set_subset_color(int index, const QColor& color) override;

		void geometry_changed() override;
	///	creates an undo entry, if no transform is currently performed.
		void visuals_changed(bool createUndoPoint = true) override;
		void marks_changed();
		void selection_changed() override;

	///	an indicator point is a point with a color independent of the grid.
	/**	You can use indicator points to highlight errors or problematic regions. */
		void add_indicator_point(float x, float y, float z, float r, float g, float b, float a);

	///	clears all indicator points
		void clear_indicator_points();

	///	returns the values of the i-th indicator point.
	/**	You should only use the returned values, if the method returns true.*/
		bool get_indicator_point(size_t index, float& x, float& y, float& z,
	                         float& r, float& g, float& b, float& a) override;

	///	the number of indicator points
		size_t num_indicator_points() override;

	//	subset handling
		bool subset_is_initialized(int index) const;
		void init_subset(int index);
		void init_subsets();

		uint get_subset_state(int index) const;
		void set_subset_state(int index, uint state);
		void enable_subset_state(int index, uint state);
		void disable_subset_state(int index, uint state);
		bool subset_state_is_enabled(int index, uint state) const;

	//	rendering
		void set_num_display_lists(int num);
		int num_display_lists() {return static_cast<int>(_display_lists.size());}
		GLuint get_display_list(int index) {return _display_lists[index];}
		void set_display_list_mode(int index, LGRenderMode mode)	{_display_modes[index] = mode;}
		int get_display_list_mode(int index) {return _display_modes[index];}

	///	set the type of elements that shall be rendered.
		void set_element_mode(uint mode)		{_element_mode = mode;}
	///	get the type of the elements that shall be rendered.
		uint get_element_mode() {return _element_mode;}
		bool volume_rendering_enabled() {return m_grid.num_volumes() > 0 && ((_element_mode & LGEM_VOLUME) == LGEM_VOLUME);}
		bool face_rendering_enabled() {return m_grid.num_faces() > 0 && (_element_mode & LGEM_FACE) == LGEM_FACE;}
		bool edge_rendering_enabled() {return m_grid.num_edges() > 0 && (_element_mode & LGEM_EDGE) == LGEM_EDGE;}
		bool vertex_rendering_enabled() {return m_grid.num_vertices() > 0 && (_element_mode & LGEM_VERTEX) == LGEM_VERTEX;}

	//	geometry info
		void update_bounding_shapes();

		ug::Sphere3& get_bounding_sphere()	{return _bound_sphere;}

		void get_bounding_box(ug::vector3& v_min_out, ug::vector3& v_max_out)
			{v_min_out = _bound_box_min; v_max_out = _bound_box_max;}

	//	undo / redo
		bool undo();
		bool redo();

	///	call this method to create an undo point if the selection changed since the last point
		void create_undo_point_if_selection_changed();

	///	creates an undo-point
	/** \note This method is automatically invoked from 'visuals_changed(true)'*/
		void create_undo_point();

	////////////////////////////////////////////////////////////////////////////
	//	TRANSFORMS
	///	Begins a new transform as indicated in the specified transform-type.
	/**	If another transform is currently active, it will be canceled.*/
		void begin_transform(TransformType tt);
	/// returns the center of the current transform
	/**	If not called between begin_transform and end_transform, the return
	 * value is undefined.*/
		ug::vector3 transform_center();

	//	grabbing
	///	specifies the offset from begin_grab, to where the objects shall be moved.
	/**	Only has effect if called between begin_grab and end_grab.*/
		void grab(const ug::vector3& offset);

	//	scaling
	///	specifies the scale-facs from begin_scale. Defines the amount to scale along each exis.
	/**	Only has effect if called between begin_grab and end_grab.*/
		void scale(const ug::vector3& scale_facs);

	///	ends the transform and either applies or reverts the changes made.
		void end_transform(bool apply);

	///	stores current vertex coordinates in a coordinate buffer
	/**	You may use 'restore_vertex_coordinates_from_buffer' to restore
	 * vertex coordinates from buffered coordinates.*/
		void buffer_current_vertex_coordinates();

	///	assigns buffered coordinates to the mesh-vertices
	/** Vertex coordinates can be buffered by a call to
	 * 'buffer_current_vertex_coordinates'.
	 *
	 * \note that buffered coordinates and mesh-vertices are associated by
	 * their index in the respective sequence. If the topology of a mesh
	 * was changed since the last call to 'buffer_current_vertex_coordinates',
	 * restoring vertex coordinates with this method may lead to unexpected results.*/
		void restore_vertex_coordinates_from_buffer();
		
	///	returns true if something was changed since the last save
		bool save_required() const					{return _save_required;}

	///	call this method to change the saveRequired flag.
	/**	Note: This method will automatically be called whenever the geometry
	 *		  or the selection changed*/
		void set_save_required(bool save_required)	{_save_required = save_required;}

	///	The action log lists all operations which were performed on the object.
		const QString& action_log() const	{return _action_log;}

	///	Adds text to the action log and emits 'actionLogChanged(str)'.
		void log_action(const QString& str);

	///	Writes a command to the action log which selects all currently selected elements
		void write_selection_to_action_log();

	///	clears the action log and emits 'actionLogCleared()'.
		void clear_action_log();

	signals:
		void actionLogChanged(const QString& new_content);
		void actionLogCleared();

	protected:
		void init();

	///	collects the vertices which will be affected and calculates the center.
	/**	Uses Grid::mark()*/
		void init_transform();

	///	loads a file from ugx without emitting signals
		bool load_ugx(const char* filename);

	protected:
		using DisplayListVec = std::vector<GLuint>;
		using DisplayModeVec = std::vector<int>;

	public:
	//protected:
		std::string _file_name;
		std::string _name;

		ug::SubsetHandler _sh_faces_for_vol_rendering;

		ug::vector3 _bound_box_min;
		ug::vector3 _bound_box_max;
		ug::Sphere3 _bound_sphere;

		DisplayListVec _display_lists;
		DisplayModeVec _display_modes;

	//	the type of the elements that shall be rendered.
		uint _element_mode;

		QColor _color;
		UndoHistory _undo_history;

		int _num_initialized_subsets;

		bool _visible;
		bool _selection_changed_since_last_undo_point;
		bool _save_required;
		
	//	currently used by LGScene to update the selection visuals only.
		int _selection_display_list_index;
		
		QString _action_log;

	private:
	//	transform
		TransformType _transform_type;
		ug::vector3 _transform_start;	// center where transform started
		ug::vector3 _transform_cur;		// center of the current selection
		ug::vector3 _transform_cur_scales;
		std::vector<ug::Vertex*> _transform_vertices;
		std::vector<ug::vector3> _transform_initial_positions;
		std::vector<ug::vector3> _vertex_coordinate_buffer;///< used in calls to 'buffer_current_vertex_coordinates' and 'restore_vertex_coordinates_from_buffer'

	protected:
		struct IndicatorPoint{
			IndicatorPoint() = default;
			IndicatorPoint(float nx, float ny, float nz, float nr,
						   float ng, float nb, float na) :
				_x(nx), _y(ny), _z(nz), _r(nr), _g(ng), _b(nb), _a(na)	{}

			float _x, _y, _z;
			float _r, _g, _b, _a;
		};

		std::vector<IndicatorPoint>	_indicator_points;
};

#endif