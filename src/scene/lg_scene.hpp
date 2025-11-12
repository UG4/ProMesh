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

#ifndef __H__LG_SCENE__
#define __H__LG_SCENE__

#include <string>
#include "lg_include.hpp"
#include "lg_object.hpp"
#include "../view3d/renderer3d_interface.hpp"
#include "scene_template.hpp"

//TODO:	remove this restriction
constexpr int MAX_NUM_CLIP_PLANES = 3;

class LGScene : public TScene<LGObject> {
	Q_OBJECT

	using BaseClass = TScene;

	public:
		LGScene();

		~LGScene() override = default;

	///	adds obj to the scene and updates its visuals.
		int add_object(LGObject* obj, bool autoDelete = true) override;

	///	updates all visuals of the scene
		void update_visuals() override;

	///	updates the visuals of the object at the given index.
		void update_visuals(int objIndex) override;

	///	updates the visuals of pObj.
		virtual void update_visuals(LGObject* pObj);
		virtual void update_selection_visuals(LGObject* obj);

		ug::Vertex* get_clicked_vertex(LGObject* pObj,
									const ug::vector3& from,
									const ug::vector3& to);

		ug::Edge* get_clicked_edge(LGObject* pObj,
									const ug::vector3& from,
									const ug::vector3& to,
									bool closestToTo = false);

		ug::Face* get_clicked_face(LGObject* p_obj,
									const ug::vector3& from,
									const ug::vector3& to);

		ug::Volume* get_clicked_volume(LGObject* p_obj,
										const ug::vector3& from,
										const ug::vector3& to);

	/**	given a rect in screen coordinates, this methods finds all
	 *	vertices which lie in that rect and writes them to vrtsOut.
	 * \return number of vertices in the rect.*/
		size_t get_vertices_in_rect(std::vector<ug::Vertex*>& vrts_out,
									LGObject* obj,
									float x_min, float y_min, float x_max, float y_max);

	/**	given a rect in screen coordinates, this methods finds all
	 *	edges which lie completly in that rect and writes them to edgesOut.
	 * \return number of edges in the rect.*/
		size_t get_edges_in_rect(std::vector<ug::Edge*>& edges_out,
								 LGObject* obj,
								 float x_min, float y_min, float x_max, float y_max);

	/**	given a rect in screen coordinates, this methods finds all
	 *	faces which lie completly in that rect and writes them to facesOut.
	 * \return number of faces in the rect.*/
		size_t get_faces_in_rect(std::vector<ug::Face*>& faces_out,
							    LGObject* obj,
							    float x_min, float y_min, float x_max, float y_max);

	/**	This algorithm uses Grid::mark.
	 *
	 *  given a rect in screen coordinates, this methods finds all
	 *	volumes which lie completly in that rect and writes them to volsOut.
	 * \return number of volumes in the rect.*/
		size_t get_volumes_in_rect(std::vector<ug::Volume*>& vols_out,
								   LGObject* obj,
								   float x_min, float y_min, float x_max, float y_max);

	/**	given a rect in screen coordinates, this methods finds all
	 *	edges which intersect that rect and writes them to edgesOut.
	 * \return number of edges in the rect.*/
		size_t get_edges_in_rect_cut(std::vector<ug::Edge*>& edges_out,
								 LGObject* obj,
								 float x_min, float y_min, float x_max, float y_max);

	/**	given a rect in screen coordinates, this methods finds all
	 *	faces which intersect that rect and writes them to facesOut.
	 * \return number of faces in the rect.*/
		size_t get_faces_in_rect_cut(std::vector<ug::Face*>& faces_out,
							    LGObject* obj,
							    float x_min, float y_min, float x_max, float y_max);

	/**	This algorithm uses Grid::mark.
	 *
	 *  given a rect in screen coordinates, this methods finds all
	 *	volumes which intersect that rect and writes them to volsOut.
	 * \return number of volumes in the rect.*/
		size_t get_volumes_in_rect_cut(std::vector<ug::Volume*>& vols_out,
								   LGObject* obj,
								   float x_min, float y_min, float x_max, float y_max);

	//	derived from IRenderer3D
	///	this method is called when the renderer shall draw its content
		void draw() override;

	///	use this method to set the front draw mode of the renderer
		void set_draw_mode_front(unsigned int drawMode) override;

	///	use this method to set the back draw mode of the renderer
		void set_draw_mode_back(unsigned int drawMode) override;

	///	the camara transform. mat is an array of 16 floats.
		void set_transform(float* mat) override;

	///	the camera parameters
		void set_camera_parameters(float fromX, float fromY, float fromZ,
	                           float dirX, float dirY, float dirZ,
	                           float upX, float upY, float upZ) override;

		void set_world_scale(float x, float y, float z) override;
		
	///	the perspective transform
		void set_perspective(float fovy, int viewWidth, int viewHeight,
	                     float zNear, float zFar) override;

		void set_ortho_perspective(float left, float right, float bottom,
	                           float top, float zNear, float zFar) override;

	///	returns the distance of the near and far clipping plane.
		void get_clip_distance_estimate(float& nearOut, float& farOut,
	                                float fromX, float fromY, float fromZ,
	                                float toX, float toY, float toZ) override;

	//	derived from TScene
		void update_visuals(ISceneObject* pObj) override;

	//	geometry
	///	returns the bounding box of the scene
		void get_bounding_box(ug::vector3& vMinOut, ug::vector3& vMaxOut);
		ug::Sphere3 get_bounding_sphere();

	//	clipping planes
		int numClipPlanes() {return MAX_NUM_CLIP_PLANES;}
		void enableClipPlane(int index, bool enable);
		bool clipPlaneIsEnabled(int index)	{return _clip_plane_enabled[index];}
		void setClipPlane(int index, const ug::Plane& plane);

	//	hide / unhide parts of the geometry
	/**	Note that this method doesn't invoke obj->visuals_changed. The caller is
	 * responsible to do so.*/
		template <typename TIterator>
		void hide_elements(LGObject* obj, TIterator elemsBegin, TIterator elemsEnd);

	/**	Note that this method doesn't invoke obj->visuals_changed. The caller is
	 * responsible to do so.*/
		template <typename TElem>
		void unhide_elements(LGObject* obj);

	/**	Note that this method doesn't invoke obj->visuals_changed. The caller is
	 * responsible to do so.*/
		void unhide_elements(LGObject* obj);

	///	sets which elements will be drawn.
	/**	\todo:	Associated code has to be cleaned up a little!*/
		void set_element_draw_mode(bool draw_vrts, bool draw_edges, bool draw_faces, bool draw_vols);

	signals:
		void geometry_changed();
		void selection_changed();

	public slots:
		void visibility_changed(ISceneObject* pObj) override;
		void color_changed(ISceneObject* pObj) override;
		void object_properties_changed();

	protected slots:
		void object_geometry_changed();
		void object_visuals_changed();
		void object_selection_changed();


	protected:
		ug::Plane near_clip_plane();
		
		void calculate_bounding_spheres(LGObject* pObj);

		void render_skeleton(LGObject* pObj);

		void render_creases(LGObject* pObj, int displayListIndex);

		void render_selection(LGObject* pObj, int displayListIndex);

		void render_points(LGObject* pObj, const ug::vector4& color,
						  ug::VertexIterator vrtsBegin,
						  ug::VertexIterator vrtsEnd,
						  ug::Grid::VertexAttachmentAccessor<ug::APosition>& aaPos);

		void render_point_subsets(LGObject* pObj, int baseDisplayListIndex = 0);

		void render_edges(LGObject* pObj, const ug::vector4& color,
						  ug::EdgeIterator edgesBegin,
						  ug::EdgeIterator edgesEnd,
						  ug::Grid::VertexAttachmentAccessor<ug::APosition>& aaPos);

		void render_edge_subsets(LGObject* pObj, int baseDisplayListIndex = 0);

		void render_triangles(LGObject* p_obj, const ug::vector4& color,
						  ug::FaceIterator tris_begin,
						  ug::FaceIterator tris_end,
						  ug::Grid::VertexAttachmentAccessor<ug::APosition>& aa_pos,
						  ug::Grid::FaceAttachmentAccessor<ug::ANormal>& aa_norm);

		void render_quadrilaterals(LGObject* p_obj,
						  const ug::vector4& color,
						  ug::FaceIterator quads_begin,
						  ug::FaceIterator quads_end,
						  ug::Grid::VertexAttachmentAccessor<ug::APosition>& aa_pos,
						  ug::Grid::FaceAttachmentAccessor<ug::ANormal>& aa_norm);

		void rerender_volumes(LGObject* p_obj,
						  const ug::vector4& color,
						  ug::VolumeIterator vols_begin,
						  ug::VolumeIterator vols_end,
						  ug::Grid::VertexAttachmentAccessor<ug::APosition>& aa_pos,
						  ug::Grid::FaceAttachmentAccessor<ug::ANormal>& aa_norm);

		void render_faces(LGObject* p_obj, ug::Grid& grid,
						  ug::SubsetHandler& sh, bool render_all = false);
		void render_volumes(LGObject* p_obj);
		void render_faces_without_clip_plane(LGObject* p_obj);
		void render_faces_with_clip_plane(LGObject* p_obj);

		bool clip_vertex(ug::Vertex* vrt,
						 ug::Grid::VertexAttachmentAccessor<ug::APosition>& aaPos);
		bool clip_edge(ug::Edge* e,
					   ug::Grid::VertexAttachmentAccessor<ug::APosition>& aaPos);
		bool clip_face(ug::Face* f, const ug::Sphere3& boundingSphere,
					   ug::Grid::VertexAttachmentAccessor<ug::APosition>& aaPos);
		bool clip_volume(ug::Volume* v, const ug::Sphere3& boundingSphere,
						 ug::Grid::VertexAttachmentAccessor<ug::APosition>& aaPos);

		ug::RelativePositionIndicator clip_sphere(const ug::Sphere3& sphere);
		ug::RelativePositionIndicator clip_point(const ug::vector3& point);

	protected:
		using AChar = ug::Attachment<char>;

	protected:
		unsigned int _draw_mode_front;
		unsigned int _draw_mode_back;
		int _view_width;
		int _view_height;
		float _mat_transform[16];
		float _fovy;
		float _aspect_ratio;
		float _z_near;
		float _z_far;
		ug::vector3 _cam_from;
		ug::vector3 _cam_dir;
		ug::vector3 _cam_up;
		ug::vector3 _world_scale;
		
	//	attachments
		ug::ASphere _a_sphere;
		ug::AInt _a_int;
		ug::ABool _a_rendered;
		ug::ABool _a_hidden;

	//	clip planes
		ug::Plane _clip_planes[MAX_NUM_CLIP_PLANES];
		bool _clip_plane_enabled[MAX_NUM_CLIP_PLANES];

	//	rendering
		bool _draw_vertices;
		bool _draw_edges;
		bool _draw_faces;
		bool _draw_volumes;
};


////////////////////////////////////////
//	include implementation
#include "lg_scene_impl.hpp"

#endif