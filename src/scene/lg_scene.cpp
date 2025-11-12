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

#include <QtOpenGL>
#include <algorithm>
#include "lg_scene.hpp"
#include "gl_includes.hpp"

using namespace std;
using namespace ug;

LGScene::LGScene() :
	_cam_from(0, 0, 0),
	_cam_dir(0, 0, -1),
	_cam_up(0, 1, 0),
	_world_scale(1, 1, 1),
	_a_hidden(true),
	_draw_vertices(true),
	_draw_edges(true),
	_draw_faces(true),
	_draw_volumes(true)
{
	_draw_mode_front = _draw_mode_back = DM_SOLID_WIRE;

	for(int i = 0; i < numClipPlanes(); ++i)
	{
		_clip_plane_enabled[i] = false;
	}
}

void LGScene::set_draw_mode_front(unsigned int drawMode)
{
	_draw_mode_front = drawMode;
	emit visuals_updated();
}

Plane LGScene::near_clip_plane()
{
	vector3 v;
	VecNormalize(v, _cam_dir);
	VecScale(v, v, _z_near);
	VecAdd(v, v, _cam_from);
	return Plane(_cam_from, _cam_dir);
}

void LGScene::set_draw_mode_back(unsigned int drawMode)
{
	_draw_mode_back = drawMode;
	emit visuals_updated();
}

void LGScene::set_transform(float* mat)
{
	memcpy(_mat_transform, mat, sizeof(float)*16);
}

void LGScene::set_camera_parameters(float fromX, float fromY, float fromZ,
								   float dirX, float dirY, float dirZ,
								   float upX, float upY, float upZ)
{
	_cam_from = vector3(fromX, fromY, fromZ);
	_cam_dir = vector3(dirX, dirY, dirZ);
	_cam_up = vector3(upX, upY, upZ);
}

void LGScene::set_world_scale(float x, float y, float z)
{
	_world_scale = vector3(x, y, z);
}

void LGScene::set_perspective(float fovy, int viewWidth, int viewHeight,
							  float zNear, float zFar)
{
	_view_width = viewWidth;
	_view_height = viewHeight;
	_fovy = fovy;
	_aspect_ratio = static_cast<float>(viewWidth) / viewHeight;
	_z_near = zNear;
	_z_far = zFar;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(_fovy, _aspect_ratio, _z_near, _z_far);
	glMatrixMode(GL_MODELVIEW);
}

void LGScene::set_ortho_perspective(float left, float right, float bottom,
									float top, float zNear, float zFar)
{
	_view_width = right-left;
	_view_height = top-bottom;
	_z_near = zNear;
	_z_far = zFar;
	_fovy = 1;
	_aspect_ratio = static_cast<float>(_view_width) / _view_height;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(left, right, bottom, top, zNear, zFar);
	glMatrixMode(GL_MODELVIEW);
}


int LGScene::add_object(LGObject* obj, bool autoDelete)
{
	if(!obj->grid().has_face_attachment(_a_sphere))
		obj->grid().attach_to_faces(_a_sphere);
	if(!obj->grid().has_volume_attachment(_a_sphere))
		obj->grid().attach_to_volumes(_a_sphere);
	if(!obj->grid().has_vertex_attachment(_a_rendered))
	{
		obj->grid().attach_to_vertices(_a_rendered);
		obj->grid().attach_to_edges(_a_rendered);
		obj->grid().attach_to_faces(_a_rendered);
		obj->grid().attach_to_volumes(_a_rendered);
	}
	if(!obj->grid().has_vertex_attachment(_a_hidden)){
		obj->grid().attach_to_vertices(_a_hidden);
		obj->grid().attach_to_edges(_a_hidden);
		obj->grid().attach_to_faces(_a_hidden);
		obj->grid().attach_to_volumes(_a_hidden);
	}

	connect(obj, &LGObject::sig_geometry_changed, this, &LGScene::object_geometry_changed);
	connect(obj, &LGObject::sig_visuals_changed, this, &LGScene::object_visuals_changed);
	connect(obj, &LGObject::sig_selection_changed, this, &LGScene::object_selection_changed);
	connect(obj, &LGObject::sig_properties_changed, this, &LGScene::object_properties_changed);

	calculate_bounding_spheres(obj);
	int retVal = BaseClass::add_object(obj, autoDelete);
	update_visuals(obj);

	emit geometry_changed();

	return retVal;
}

void LGScene::visibility_changed(ISceneObject* pObj)
{
//	update the geometry if volumes are contained

	if(auto* lgObj = dynamic_cast<LGObject*>(pObj))
	{
		if(lgObj->volume_rendering_enabled())
			update_visuals(lgObj);//render_volumes(lgObj);
		else if(lgObj->face_rendering_enabled())
			update_visuals(lgObj);//render_faces(lgObj, lgObj->grid(), lgObj->subset_handler());
	}

	emit geometry_changed();
	emit visuals_updated();
}

void LGScene::color_changed(ISceneObject* pObj)
{
	emit visuals_updated();
}

void LGScene::object_geometry_changed()
{
//	LGObject* obj = qobject_cast<LGObject*>(sender());
	auto* obj = dynamic_cast<LGObject*>(sender());
	if(obj){
		calculate_bounding_spheres(obj);
		Grid& g = obj->grid();
		CalculateFaceNormals(g, g.begin<Face>(), g.end<Face>(), aPosition, aNormal);
		update_visuals(obj);
		emit geometry_changed();
	}
}

void LGScene::object_visuals_changed()
{
	auto* obj = dynamic_cast<LGObject*>(sender());
	if(obj){
		update_visuals(obj);
	}
}

void LGScene::object_selection_changed()
{
	auto* obj = dynamic_cast<LGObject*>(sender());
	if(obj){
		update_selection_visuals(obj);
		emit selection_changed();
	}
}

void LGScene::object_properties_changed()
{
	if(auto* obj = dynamic_cast<ISceneObject*>(sender())){
		emit IScene::object_properties_changed(obj);
	}
}

void LGScene::get_bounding_box(vector3& vMinOut, vector3& vMaxOut)
{
//	get the bounding box that surrounds all objects
	vMinOut = vMaxOut = vector3(0, 0, 0);

	bool gotOne = false;
	for(int i = 0; i < num_objects(); ++i)
	{
		LGObject* obj = get_object(i);
		if(obj->is_visible())
		{
			if(!gotOne)
			{
				obj->get_bounding_box(vMinOut, vMaxOut);
				gotOne = true;
			}
			else
			{
				vector3 tMin, tMax;
				obj->get_bounding_box(tMin, tMax);
				VecCompMin(vMinOut, vMinOut, tMin);
				VecCompMax(vMaxOut, vMaxOut, tMax);
			}
		}
	}

	for(int i = 0; i < 3; ++i){
		vMinOut[i] *= _world_scale[i];
		vMaxOut[i] *= _world_scale[i];
	}
}

Sphere3 LGScene::get_bounding_sphere()
{
//	approximate center and radius
	vector3 vMin, vMax;
	get_bounding_box(vMin, vMax);

	vector3 vCenter;
	VecAdd(vCenter, vMin, vMax);
	VecScale(vCenter, vCenter, 0.5);

	return Sphere3(vCenter, 0.5 * VecDistance(vMin, vMax));
}

//	index from 0 to 2: xy, xz, yz
void LGScene::enableClipPlane(int index, bool enable)
{
	_clip_plane_enabled[index] = enable;
}

//	index from 0 to 2: xy, xz, yz
//	values from 0 to 1.
void LGScene::setClipPlane(int index, const Plane& plane)
{
	_clip_planes[index] = plane;
}

void LGScene::calculate_bounding_spheres(LGObject* pObj)
{
	Grid& grid = pObj->grid();

//	calculate bounding-spheres.
	Grid::VertexAttachmentAccessor aaPos(grid, aPosition);
	Grid::FaceAttachmentAccessor aaSphereFACE(grid, _a_sphere);
	Grid::VolumeAttachmentAccessor aaSphereVOL(grid, _a_sphere);

	for(FaceIterator iter = grid.faces_begin(); iter != grid.faces_end(); ++iter)
		CalculateBoundingSphere(aaSphereFACE[*iter], *iter, aaPos);
	for(VolumeIterator iter = grid.volumes_begin(); iter != grid.volumes_end(); ++iter)
		CalculateBoundingSphere(aaSphereVOL[*iter], *iter, aaPos);
}

bool LGScene::clip_vertex(Vertex* v, Grid::VertexAttachmentAccessor<APosition>& aaPos)
{
//	exact version
	for(int i = 0; i < numClipPlanes(); ++i){
		if(clipPlaneIsEnabled(i)){
			if(PlanePointTest(_clip_planes[i], aaPos[v]) == RPI_OUTSIDE)
				return true;
		}
	}

	return false;
}

bool LGScene::clip_edge(Edge* e, Grid::VertexAttachmentAccessor<APosition>& aaPos)
{
//	exact version
	for(int i = 0; i < numClipPlanes(); ++i){
		if(clipPlaneIsEnabled(i)){
			if(ClipEdge(e, _clip_planes[i], aaPos))
				return true;
		}
	}

	return false;
}

bool LGScene::clip_face(Face* f, const Sphere3& boundingSphere,
						Grid::VertexAttachmentAccessor<APosition>& aaPos)
{
//	exact version
	for(int i = 0; i < numClipPlanes(); ++i){
		if(clipPlaneIsEnabled(i)){
			if(ClipFace(f, boundingSphere, _clip_planes[i], aaPos))
				return true;
		}
	}

	return false;
}

bool LGScene::clip_volume(Volume* v, const Sphere3& boundingSphere,
						Grid::VertexAttachmentAccessor<APosition>& aaPos)
{
//	exact version
	for(int i = 0; i < numClipPlanes(); ++i){
		if(clipPlaneIsEnabled(i)){
			if(ClipVolume(v, boundingSphere, _clip_planes[i], aaPos))
				return true;
		}
	}

	return false;
}

RelativePositionIndicator LGScene::clip_sphere(const Sphere3& sphere)
{
	RelativePositionIndicator retVal = RPI_INSIDE;
	for(int i = 0; i < numClipPlanes(); ++i)
	{
		if(clipPlaneIsEnabled(i))
		{
			RelativePositionIndicator nVal = PlaneSphereTest(_clip_planes[i], sphere);
			if(nVal > retVal)
				retVal = nVal;
			if(retVal == RPI_OUTSIDE)
				return retVal;
		}
	}
	return retVal;
}

RelativePositionIndicator LGScene::clip_point(const vector3& point)
{
	RelativePositionIndicator retVal = RPI_INSIDE;
	for(int i = 0; i < numClipPlanes(); ++i)
	{
		if(clipPlaneIsEnabled(i))
		{
			RelativePositionIndicator nVal = PlanePointTest(_clip_planes[i], point);
			if(nVal > retVal)
				retVal = nVal;
			if(retVal == RPI_OUTSIDE)
				return retVal;
		}
	}
	return retVal;
}

void LGScene::get_clip_distance_estimate(float& nearOut, float& farOut,
										float fromX, float fromY, float fromZ,
										float toX, float toY, float toZ)
{
//	near and far clipping plane
	vector3 from(fromX, fromY, fromZ);
	vector3 to(toX, toY, toZ);
	vector3 dir;
	VecSubtract(dir, to, from);

	Sphere3 bndSphere = get_bounding_sphere();
	if(bndSphere.get_radius() < SMALL)
		bndSphere.set_radius(1);

	float distToCenter = DistancePointToPlane(bndSphere.get_center(), from, dir);
	farOut = distToCenter + bndSphere.get_radius() * 1.01;
	nearOut = distToCenter - bndSphere.get_radius() * 1.01;

	bool correctDistX = true;

//	if dist.x() < 0 (camera is inside the bounding sphere) we will check
//	for active clip-planes and adjust the distance accordingly.
	if(nearOut < 0)
	{
		for(int i = 0; i < numClipPlanes(); ++i)
		{
			if(clipPlaneIsEnabled(i))
			{
				if(nearOut < 0)
					nearOut = static_cast<float>(PlanePointDistance(_clip_planes[i], from)) * 0.1;
				else
				{
					float tDist = static_cast<float>(PlanePointDistance(_clip_planes[i], from)) * 0.1;
					nearOut = min(nearOut, tDist);
				}

				correctDistX = false;
				if(nearOut < 0.00001f)
					nearOut = 0.00001f;
			}
		}
	}

	if(correctDistX)
	{
		number minBorder = bndSphere.get_radius() * 0.001f;
		if(minBorder < 0.00001f)
			minBorder = 0.00001f;
		if(nearOut < minBorder)
			nearOut = minBorder;
	}

	nearOut = min(nearOut, (float)VecLength(dir) / 100.f);
}

static QColor ColorAdjust(const QColor& c)
{
	static const int colDif = 50;
	if(c.redF() + c.blueF() + c.greenF() > 2.6) {
		return QColor (	max<int>(0, c.red() - colDif),
						max<int>(0, c.green() - colDif),
						max<int>(0, c.blue() - colDif),
						c.alpha());
	}
	else {
		return QColor (	min<int>(255, c.red() + colDif),
						min<int>(255, c.green() + colDif),
						min<int>(255, c.blue() + colDif),
						c.alpha());
	}
}

void LGScene::draw()
{
	static GLfloat lightDirection[] = { 0, 0.0f, 1.0f, 0.0f };
	static GLfloat lightDirectionInv[] = { 0, 0.0f, -1.0f, 0.0f };
	static GLfloat lightAmbientLow[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
	static GLfloat lightAmbientFull[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static GLfloat lightDiffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static GLfloat lightDiffuseInv[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	for(int i = 0; i < num_objects(); ++i)
	{
	//	init settings
		glDisable(GL_CULL_FACE);
		glFrontFace(GL_CW);

		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glEnable(GL_CULL_FACE);
		glEnable(GL_RESCALE_NORMAL);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		LGObject* obj = get_object(i);
		if(obj->is_visible())
		{
		//	first we'll check which drawmodes are required
			bool drawDoublePassShaded = false;
			bool drawSinglePassColor = false;
			bool drawDoublePassColor = false;
			bool drawSinglePassNoLight = false;
			for(int j = 0; j < obj->num_display_lists(); ++j){
				drawDoublePassShaded |= obj->get_display_list_mode(j) == LGRM_DOUBLE_PASS_SHADED;
				drawSinglePassColor |= obj->get_display_list_mode(j) == LGRM_SINGLE_PASS_COLOR;
				drawDoublePassColor |= obj->get_display_list_mode(j) == LGRM_DOUBLE_PASS_COLOR;
				drawSinglePassNoLight |= obj->get_display_list_mode(j) == LGRM_SINGLE_PASS_NO_LIGHT;
			}

		//	draw double-pass-shaded
			if(drawDoublePassShaded){

				glPolygonOffset(1, 2);
				int drawMode[2];
				drawMode[0] = _draw_mode_front;
				drawMode[1] = _draw_mode_back;

			//	we'll do this twice. for both orientations.
			//	draw back first. wire-frame is rendered without z-buffer-write.
				for(int iPass = 1; iPass >= 0; --iPass)
				{
				//	if the draw-mode is set to DM_NONE we'll continue right away
					if(drawMode[iPass] == DM_NONE) {
						//ø std::cout << "draw_mode ipass continue" << std::endl;
						continue;
					}

				//	init world-matrix
					glMatrixMode(GL_MODELVIEW);
					glLoadIdentity();

					if(iPass == 0) 	{ //	normal orientation
						glLightfv( GL_LIGHT0, GL_POSITION, lightDirection);
						glLightfv( GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
						glCullFace(GL_BACK);
						glFrontFace(GL_CCW); // or GL_CW depending on winding

					} else { //	inverted orientation
						glLightfv( GL_LIGHT0, GL_POSITION, lightDirectionInv);
						glLightfv( GL_LIGHT0, GL_DIFFUSE, lightDiffuseInv);
						glCullFace(GL_FRONT);
						glFrontFace(GL_CCW); // or GL_CW depending on winding

					}

					glMultMatrixf(_mat_transform);

					QColor objCol = obj->get_color();

//TODO: either iterate over subsets or add a visible state and colors per display-list
					for(int j = 0; j < obj->num_display_lists(); ++j)
					{
					//	currently this works ... Add a color per display list later on
					//	and remove this HACK!
						int si = j % obj->num_subsets();
						if(/*obj->subset_is_visible(si) &&*/
						   (obj->get_display_list_mode(j) == LGRM_DOUBLE_PASS_SHADED))
						{
							if(drawMode[iPass] & DM_SOLID)
							{
							//	draw solid
								glDepthMask(true);
								glDisable(GL_BLEND);
								glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

								QColor sCol = obj->get_subset_color(si);
								GLfloat faceColor[4] = {GLfloat(objCol.redF() * sCol.redF()),
														GLfloat(objCol.greenF() * sCol.greenF()),
														GLfloat(objCol.blueF() * sCol.blueF()),
														GLfloat(1)};

								glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE, faceColor);
								glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, faceColor);
								glCallList(obj->get_display_list(j));
							}

							if(drawMode[iPass] & DM_WIRE)
							{
							//	draw wire
							//	check whether we have to use the z-buffer or not.
								GLfloat wireColor[4] = {0.4f, 0.4f, 0.4f, 1.0f};
								if(drawMode[iPass] & DM_SOLID)
								{
									glDepthMask(false);
									glEnable(GL_BLEND);
									glEnable(GL_LINE_SMOOTH);
									glLineWidth(1.f);
								}
								else
								{
									glDepthMask(true);
									glDisable(GL_BLEND);
									glDisable(GL_LINE_SMOOTH);
									glLineWidth(1.f);
								}
								glDisable(GL_POLYGON_OFFSET_FILL);
								glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

								glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE, wireColor);
								glCallList(obj->get_display_list(j));
								glEnable(GL_POLYGON_OFFSET_FILL);
							}
						}
					}
				}
				glDepthMask(true);
			}

// 		//	draw single-pass-color
			if(drawSinglePassColor || drawDoublePassColor){
				for(int j = 0; j < obj->num_display_lists(); ++j)
				{
					//ø std::cout << "j = " << j << std::endl;
//TODO:	add display-list visibility
					if((obj->get_display_list_mode(j) == LGRM_SINGLE_PASS_COLOR) ||
					   (obj->get_display_list_mode(j) == LGRM_DOUBLE_PASS_COLOR))
					{
						glMatrixMode(GL_MODELVIEW);
						glLoadIdentity();
						glMultMatrixf(_mat_transform);

						glLineWidth(2.f);
						glEnable(GL_BLEND);
						glEnable(GL_LINE_SMOOTH);
						glDisable(GL_LIGHTING);
						//glDisable(GL_POLYGON_OFFSET_FILL);
						glPolygonOffset(1, 1);
						glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

						if(drawDoublePassColor) {
							glDisable(GL_CULL_FACE);
						} else {
							glEnable(GL_CULL_FACE);
						}

						glCallList(obj->get_display_list(j));

						glEnable(GL_POLYGON_OFFSET_FILL);
						glEnable(GL_LIGHTING);
					}
				}
			}


			//ø std::cout << "drawSinglePassNoLight = " << drawSinglePassNoLight << std::endl;
//	draw single-pass-no-light
			if(drawSinglePassNoLight){
				QColor objCol = obj->get_color();

				//for(int j = 0; j < obj->num_subsets(); ++j)
				//{
				for(int j = 0; j < obj->num_display_lists(); ++j)
				{
				//	currently this works ... Add a color per display list later on
				//	and remove this HACK!
					int si = j % obj->num_subsets();

//TODO:	add display-list visibility
					if((obj->get_display_list_mode(j) == LGRM_SINGLE_PASS_NO_LIGHT))
					{
						glMatrixMode(GL_MODELVIEW);
						glLoadIdentity();
						glMultMatrixf(_mat_transform);

						glLineWidth(2.f);
						//glDisable(GL_BLEND);
						glEnable(GL_BLEND);
						glEnable(GL_LINE_SMOOTH);
						glEnable(GL_LIGHTING);
						glDisable(GL_LIGHT0);
						glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lightAmbientFull);

						//glLightfv( GL_LIGHT0, GL_DIFFUSE, lightAmbient);
						//glDisable(GL_POLYGON_OFFSET_FILL);
						glPolygonOffset(1, 1);
						glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
/*
						if(drawDoublePassColor)
							glDisable(GL_CULL_FACE);
						else
							glEnable(GL_CULL_FACE);
*/
						//glEnable(GL_COLOR_MATERIAL);
						//glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

						glColor3f (	objCol.redF(),
									objCol.greenF(),
									objCol.blueF());
						
						QColor sCol = ColorAdjust(obj->get_subset_color(si));
						GLfloat glCol[4] = {GLfloat(sCol.redF()),
											GLfloat(sCol.greenF()),
											GLfloat(sCol.blueF()),
											GLfloat(1)};

						glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE, glCol);
						glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, glCol);

						glCallList(obj->get_display_list(j));

						glEnable(GL_POLYGON_OFFSET_FILL);
						glEnable(GL_LIGHTING);
						glEnable(GL_LIGHT0);
						glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lightAmbientLow);
					}
				}
			}
		}
	}
}

void LGScene::update_visuals()
{
	for(int i = 0; i < num_objects(); ++i)
		update_visuals(i);
}

void LGScene::update_visuals(int objIndex)
{
	update_visuals(get_object(objIndex));
}

void LGScene::update_visuals(ISceneObject* pObj)
{
	auto* pLGObj = dynamic_cast<LGObject*>(pObj);
	if(pLGObj)
		update_visuals(pLGObj);
	emit visuals_updated();
}

void LGScene::update_visuals(LGObject* pObj)
{
//	check whether a clip plane is enabled
	bool clipPlaneEnabled = false;
	for(int i = 0; i < numClipPlanes(); ++i)
	{
		if(clipPlaneIsEnabled(i))
		{
			clipPlaneEnabled = true;
			break;
		}
	}

//	all elements are initially undrawn
	Grid& grid = pObj->grid();
	Grid::VertexAttachmentAccessor aaRenderedVRT(grid, _a_rendered);
	Grid::EdgeAttachmentAccessor aaRenderedEDGE(grid, _a_rendered);
	Grid::FaceAttachmentAccessor aaRenderedFACE(grid, _a_rendered);
	Grid::VolumeAttachmentAccessor aaRenderedVOL(grid, _a_rendered);

	SetAttachmentValues(aaRenderedVRT, grid.vertices_begin(), grid.vertices_end(), false);
	SetAttachmentValues(aaRenderedEDGE, grid.edges_begin(), grid.edges_end(), false);
	SetAttachmentValues(aaRenderedFACE, grid.faces_begin(), grid.faces_end(), false);
	SetAttachmentValues(aaRenderedVOL, grid.volumes_begin(), grid.volumes_end(), false);

//	calculate the number of required display lists
	int numSubsets = pObj->subset_handler().num_subsets();
	int numDisplayLists = 0;

	bool drawVolumes = _draw_volumes && pObj->grid().num_volumes() > 0;
	bool drawFaces = _draw_faces && pObj->grid().num_faces() > 0 && !drawVolumes;
	bool drawEdges = _draw_edges && pObj->grid().num_edges() > 0;
	bool drawVertices = _draw_vertices && pObj->grid().num_vertices() > 0;

	if(drawVolumes)
		numDisplayLists += numSubsets;
	if(drawFaces)
		numDisplayLists += numSubsets;
	if(drawEdges)
		numDisplayLists += numSubsets;
	if(drawVertices)
		numDisplayLists += numSubsets;

	bool bDrawSelection = !pObj->selector().empty();
	if(bDrawSelection)
		numDisplayLists++;

	bool bDrawMarks = (pObj->crease_handler().num<Vertex>(REM_FIXED) > 0)
					  || (pObj->crease_handler().num<Edge>(REM_CREASE) > 0);
	if(bDrawMarks)
		numDisplayLists++;

	pObj->set_num_display_lists(numDisplayLists);

	int curDisplayListIndex = 0;

//	perform the rendering
	//if((pObj->volume_rendering_enabled()))
	if(drawVolumes)
	{
	//	render volumes
		assert(curDisplayListIndex + numSubsets < numDisplayLists);
		render_volumes(pObj);
		curDisplayListIndex += numSubsets;
	}
	//else if((pObj->face_rendering_enabled()))
	if(drawFaces)
	{
		assert(curDisplayListIndex + numSubsets < numDisplayLists);
	//	render faces
		if(clipPlaneEnabled)
			render_faces_with_clip_plane(pObj);
		else
			render_faces_without_clip_plane(pObj);
		curDisplayListIndex += numSubsets;
	}
	//if(pObj->edge_rendering_enabled()){
	if(drawEdges){
		assert(curDisplayListIndex + numSubsets < numDisplayLists);
		render_edge_subsets(pObj, curDisplayListIndex);
		curDisplayListIndex += numSubsets;
	}
	//if(pObj->vertex_rendering_enabled()){
	if(drawVertices){
		// UG_ASSERT(curDisplayListIndex + numSubsets < numDisplayLists,
		// 		  "curDisplayListIndex: " << curDisplayListIndex
		// 		  << ", numSubsets: " << numSubsets
		// 		  << ", numDisplayLists: " << numDisplayLists);
		render_point_subsets(pObj, curDisplayListIndex);
		curDisplayListIndex += numSubsets;
	}

//	if something is selected, we'll render it
	if(bDrawSelection){
		assert(curDisplayListIndex < numDisplayLists);
		render_selection(pObj, curDisplayListIndex);
		pObj->_selection_display_list_index = curDisplayListIndex;
		++curDisplayListIndex;
	}

//	if there are crease edges, we'll render them
	if(bDrawMarks){
		assert(curDisplayListIndex < numDisplayLists);
		render_creases(pObj, curDisplayListIndex);
		++curDisplayListIndex;
	}

	emit visuals_updated();
}

void LGScene::update_selection_visuals(LGObject* obj)
{
	if(obj->_selection_display_list_index < 0)
		update_visuals(obj);
	else{
		render_selection(obj, obj->_selection_display_list_index);
		emit visuals_updated();
	}
}

void LGScene::render_skeleton(LGObject* pObj)
{
	Grid& grid = pObj->grid();
	Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);

//	iterate through all subsets
	pObj->set_display_list_mode(0, LGRM_SINGLE_PASS_COLOR);
	GLuint displayList = pObj->get_display_list(0);
	glDeleteLists(displayList, 1);
	glNewList(displayList, GL_COMPILE);

//	draw edges
	render_edges(pObj, vector4(0.1f, 1.f, 0.1f, 1.f), grid.edges_begin(), grid.edges_end(), aaPos);

//	draw points
	render_points(pObj, vector4(0.1f, 0.1f, 1.f, 1.f), grid.vertices_begin(), grid.vertices_end(), aaPos);

	glEndList();
}

void LGScene::render_creases(LGObject* pObj, int displayListIndex)
{
	Grid& grid = pObj->grid();
	SubsetHandler& sh = pObj->crease_handler();
	Grid::VertexAttachmentAccessor aaPos(grid, aPosition);

	GLuint displayList = pObj->get_display_list(displayListIndex);
	pObj->set_display_list_mode(displayListIndex, LGRM_SINGLE_PASS_COLOR);
	glDeleteLists(displayList, 1);
	glNewList(displayList, GL_COMPILE);

//	draw vertices
	render_points(pObj, vector4(0.1f, 0.1f, 0.9f, 1.f), sh.begin<Vertex>(REM_FIXED), sh.end<Vertex>(REM_FIXED), aaPos);
//	draw edges
	render_edges(pObj, vector4(0.1f, 0.1f, 0.9f, 1.f), sh.begin<Edge>(REM_CREASE), sh.end<Edge>(REM_CREASE), aaPos);

	glEndList();
}

void LGScene::render_selection(LGObject* pObj, int displayListIndex)
{
	Grid& grid = pObj->grid();
	Selector& sel = pObj->selector();
	Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);
	Grid::FaceAttachmentAccessor<ANormal> aaNorm(grid, aNormal);

	GLuint displayList = pObj->get_display_list(displayListIndex);
	pObj->set_display_list_mode(displayListIndex, LGRM_DOUBLE_PASS_COLOR);
	glDeleteLists(displayList, 1);
	glNewList(displayList, GL_COMPILE);

	vector4 vrtColor(1.f, 0.7f, 0.1f, 1.0);
	vector4 edgeColor(1.f, 0.7f, 0.1f, 1.0);
	vector4 faceColor(1.f, 0.7f, 0.1f, 0.5);
	vector4 volColor(1.f, 0.7f, 0.1f, 0.3);

	bool drawVolumes = _draw_volumes && pObj->grid().num_volumes() > 0;
	bool drawFaces = _draw_faces && pObj->grid().num_faces() > 0 && !drawVolumes;
	bool drawEdges = _draw_edges && pObj->grid().num_edges() > 0;
	bool drawVertices = _draw_vertices && pObj->grid().num_vertices() > 0;

//	draw faces
	//if(pObj->face_rendering_enabled()){
	if(drawFaces || drawVolumes){
		render_triangles(pObj, faceColor, sel.begin<Triangle>(), sel.end<Triangle>(), aaPos, aaNorm);

		if(sel.num<ConstrainedTriangle>() > 0)
			render_triangles(pObj, faceColor, sel.begin<ConstrainedTriangle>(), sel.end<ConstrainedTriangle>(), aaPos, aaNorm);

		if(sel.num<ConstrainingTriangle>() > 0)
			render_triangles(pObj, faceColor, sel.begin<ConstrainingTriangle>(), sel.end<ConstrainingTriangle>(), aaPos, aaNorm);

		// todo self.num<Quadrilateral> > 0
		render_quadrilaterals(pObj, faceColor, sel.begin<Quadrilateral>(), sel.end<Quadrilateral>(), aaPos, aaNorm);

		if(sel.num<ConstrainedQuadrilateral>() > 0)
			render_quadrilaterals(pObj, faceColor, sel.begin<ConstrainedQuadrilateral>(), sel.end<ConstrainedQuadrilateral>(), aaPos, aaNorm);

		if(sel.num<ConstrainingQuadrilateral>() > 0)
			render_quadrilaterals(pObj, faceColor, sel.begin<ConstrainingQuadrilateral>(), sel.end<ConstrainingQuadrilateral>(), aaPos, aaNorm);
	}

//	draw volumes
//	draw volumes after faces, since the draw-order is important.
	//if(pObj->volume_rendering_enabled()){
	if(drawVolumes){
		rerender_volumes(pObj, volColor, sel.begin<Volume>(), sel.end<Volume>(), aaPos, aaNorm);
	}

//	draw edges
	if(drawEdges || drawFaces || drawVolumes){
		render_edges(pObj, edgeColor, sel.begin<Edge>(), sel.end<Edge>(), aaPos);
	}

//	draw points
	if(drawVertices || drawEdges || drawFaces || drawVolumes){
		render_points(pObj, vrtColor, sel.begin<Vertex>(), sel.end<Vertex>(), aaPos);
	}

	glEndList();
}

void LGScene::render_points(LGObject* pObj, const vector4& color,
							  VertexIterator vrtsBegin,
							  VertexIterator vrtsEnd,
							  Grid::VertexAttachmentAccessor<APosition>& aaPos)
{
	Grid::VertexAttachmentAccessor aaRenderedVRT(pObj->grid(), _a_rendered);

	glColor4f(color.x(), color.y(), color.z(), color.w());
	glPointSize(5.f);

	glBegin(GL_POINTS);

	for(VertexIterator iter = vrtsBegin;
		iter != vrtsEnd; ++iter)
	{
		if(aaRenderedVRT[*iter]){
			vector3& v = aaPos[*iter];
			glVertex3f(v.x(), v.y(), v.z());
		}
	}

	glEnd();
}

void LGScene::render_point_subsets(LGObject* pObj, int baseDisplayListIndex)
{
	Grid& grid = pObj->grid();
	SubsetHandler& sh = pObj->subset_handler();
	Grid::VertexAttachmentAccessor aaPos(grid, aPosition);
	Grid::VertexAttachmentAccessor aaRenderedVRT(grid, _a_rendered);
	Grid::VertexAttachmentAccessor aaHiddenVRT(grid, _a_hidden);

	for(int i = 0; i < sh.num_subsets(); ++i)
	{
	//	set up open-gl display lists
		int dispListIndex = baseDisplayListIndex + i;
		GLuint displayList = pObj->get_display_list(dispListIndex);
		pObj->set_display_list_mode(dispListIndex, LGRM_SINGLE_PASS_NO_LIGHT);
		glDeleteLists(displayList, 1);
		glNewList(displayList, GL_COMPILE);

		if(!pObj->subset_is_visible(i)){
			glEndList();
			continue;
		}

		//SubsetInfo& si = sh.subset_info(i);
		//render_edges(pObj, si.color, sh.begin<Edge>(i), sh.end<Edge>(i), aaPos);
		glPointSize(5.f);
		glBegin(GL_POINTS);
		glColor4f(1., 1., 1., 1.);
		for(VertexIterator iter = sh.begin<Vertex>(i);
			iter != sh.end<Vertex>(i); ++iter)
		{
			Vertex* vrt = *iter;
			if((!aaHiddenVRT[vrt]) && (!clip_vertex(vrt, aaPos))){
				aaRenderedVRT[vrt] = true;
				vector3& v = aaPos[vrt];
				glVertex3f(v.x(), v.y(), v.z());
			}
		}

		glEnd();

		glEndList();
	}
}

void LGScene::render_edges(LGObject* pObj, const vector4& color,
						  EdgeIterator edgesBegin,
						  EdgeIterator edgesEnd,
						  Grid::VertexAttachmentAccessor<APosition>& aaPos)
{
	Grid& grid = pObj->grid();
	Grid::EdgeAttachmentAccessor aaRenderedEDGE(grid, _a_rendered);

//	draw edges
	glColor4f(color.x(), color.y(), color.z(), color.w());
	glBegin(GL_LINES);

	for(EdgeIterator iter = edgesBegin;
		iter != edgesEnd; ++iter)
	{
		Edge* e = *iter;
		if(aaRenderedEDGE[e]){
			for(int i = 0; i < 2; ++i)
			{
				vector3& v = aaPos[e->vertex(i)];
				glVertex3f(v.x(), v.y(), v.z());
			}
		}
	}

	glEnd();
}

void LGScene::render_edge_subsets(LGObject* pObj, int baseDisplayListIndex)
{
	Grid& grid = pObj->grid();
	SubsetHandler& sh = pObj->subset_handler();
	Grid::VertexAttachmentAccessor aaPos(grid, aPosition);
	Grid::VertexAttachmentAccessor aaRenderedVRT(grid, _a_rendered);
	Grid::EdgeAttachmentAccessor aaRenderedEDGE(grid, _a_rendered);
	Grid::EdgeAttachmentAccessor aaHiddenEDGE(grid, _a_hidden);

	for(int i = 0; i < sh.num_subsets(); ++i)
	{
	//	set up open-gl display lists
		int dispListIndex = baseDisplayListIndex + i;
		GLuint displayList = pObj->get_display_list(dispListIndex);
		pObj->set_display_list_mode(dispListIndex, LGRM_SINGLE_PASS_NO_LIGHT);
		glDeleteLists(displayList, 1);
		glNewList(displayList, GL_COMPILE);

		if(!pObj->subset_is_visible(i)){
			glEndList();
			continue;
		}

		//SubsetInfo& si = sh.subset_info(i);
		//render_edges(pObj, si.color, sh.begin<Edge>(i), sh.end<Edge>(i), aaPos);
		glBegin(GL_LINES);
		glColor4f(1., 1., 1., 1.);
		for(EdgeIterator iter = sh.begin<Edge>(i);
			iter != sh.end<Edge>(i); ++iter)
		{
			Edge* e = *iter;
			if((!aaHiddenEDGE[e]) && (!clip_edge(e, aaPos))){
				aaRenderedEDGE[e] = true;
				for(int i = 0; i < 2; ++i){
					aaRenderedVRT[e->vertex(i)] = true;
					vector3& v = aaPos[e->vertex(i)];
					glVertex3f(v.x(), v.y(), v.z());
				}
			}
		}

		glEnd();

		glEndList();
	}
}

void LGScene::render_triangles(LGObject* p_obj, const vector4& color,
						  FaceIterator tris_begin,
						  FaceIterator tris_end,
						  Grid::VertexAttachmentAccessor<APosition>& aa_pos,
						  Grid::FaceAttachmentAccessor<ANormal>& aa_norm)
{
	Grid::FaceAttachmentAccessor aa_rendered_face(p_obj->grid(), _a_rendered);

	glColor4f(color.x(), color.y(), color.z(), color.w());
	glBegin(GL_TRIANGLES);

	for(FaceIterator iter = tris_begin; iter != tris_end; ++iter)
	{
		Face* tri = *iter;

		if(aa_rendered_face[tri]){
			vector3& n = aa_norm[tri];
			glNormal3f(n.x(), n.y(), n.z());

			for(int i = 0; i < 3; ++i)
			{
				vector3& v = aa_pos[tri->vertex(i)];
				glVertex3f(v.x(), v.y(), v.z());
			}
		}
	}

	glEnd();
}

void LGScene::render_quadrilaterals(LGObject* p_obj,
						  const vector4& color,
						  FaceIterator quads_begin,
						  FaceIterator quads_end,
						  Grid::VertexAttachmentAccessor<APosition>& aa_pos,
						  Grid::FaceAttachmentAccessor<ANormal>& aa_norm)
{
	Grid::FaceAttachmentAccessor aa_rendered_face(p_obj->grid(), _a_rendered);

	glColor4f(color.x(), color.y(), color.z(), color.w());
	glBegin(GL_QUADS);

	for(FaceIterator iter = quads_begin; iter != quads_end; ++iter)
	{
		Face* q = *iter;

		if(aa_rendered_face[q]){
			vector3& n = aa_norm[q];
			glNormal3f(n.x(), n.y(), n.z());

			for(int i = 0; i < 4; ++i)
			{
				vector3& v = aa_pos[q->vertex(i)];
				glVertex3f(v.x(), v.y(), v.z());
			}
		}
	}

	glEnd();
}

void LGScene::rerender_volumes(LGObject* p_obj,
						  const vector4& color,
						  VolumeIterator vols_begin,
						  VolumeIterator vols_end,
						  Grid::VertexAttachmentAccessor<APosition>& aa_pos,
						  Grid::FaceAttachmentAccessor<ANormal>& aa_norm)
{
	Grid& grid = p_obj->grid();
	Grid::FaceAttachmentAccessor aa_rendered_face(grid, _a_rendered);
	Grid::VolumeAttachmentAccessor aa_rendered_vol(grid, _a_rendered);

//	collect all triangles and quadrilaterals that have to be rendered
	vector<Face*> tris;
	vector<Face*> quads;
	Grid::face_traits::secure_container	ass_faces;
	for(VolumeIterator iter = vols_begin; iter != vols_end; ++iter)
	{
		Volume* vol = *iter;
		if(aa_rendered_vol[vol]){
		//	iterate through the volumes faces
			grid.associated_elements(ass_faces, vol);
			// Grid::AssociatedFaceIterator fEnd = grid.associated_faces_end(vol);
			// for(Grid::AssociatedFaceIterator fIter = grid.associated_faces_begin(vol);
			// 	fIter != fEnd; ++fIter)
			for(size_t iface = 0; iface < ass_faces.size(); ++iface){
				// Face* f = *fIter;
				Face* f = ass_faces[iface];
				if(aa_rendered_face[f]){
					if(f->num_vertices() == 3)
						tris.push_back(f);
					else{
						assert(f->num_vertices() == 4 && "unsupported face type");
						quads.push_back(f);
					}
				}
			}
		}
	}

	glColor4f(color.x(), color.y(), color.z(), color.w());

	if(!tris.empty()){
		glBegin(GL_TRIANGLES);

		for(size_t j = 0; j < tris.size(); ++j){
			Face* tri = tris[j];

			vector3& n = aa_norm[tri];
			glNormal3f(n.x(), n.y(), n.z());

			for(int i = 0; i < 3; ++i){
				vector3& v = aa_pos[tri->vertex(i)];
				glVertex3f(v.x(), v.y(), v.z());
			}
		}

		glEnd();
	}

	if(!quads.empty()){
		glBegin(GL_QUADS);

		for(size_t j = 0; j < quads.size(); ++j){
			Face* q = quads[j];

			vector3& n = aa_norm[q];
			glNormal3f(n.x(), n.y(), n.z());

			for(int i = 0; i < 4; ++i){
				vector3& v = aa_pos[q->vertex(i)];
				glVertex3f(v.x(), v.y(), v.z());
			}
		}

		glEnd();
	}
}

void LGScene::render_faces(LGObject* p_obj, Grid& grid, SubsetHandler& sh, bool render_all)
{
	PROFILE_FUNC();
//	this method allows to fill the contents of the display lists
//	with differing grids and subsets.
	Grid::VertexAttachmentAccessor aa_pos(grid, aPosition);
	Grid::FaceAttachmentAccessor aa_norm(grid, aNormal);
	Grid::VertexAttachmentAccessor aa_rendered_vrt(grid, _a_rendered);
	Grid::EdgeAttachmentAccessor aa_rendered_edge(grid, _a_rendered);
	Grid::FaceAttachmentAccessor aa_rendered_face(grid, _a_rendered);
	Grid::FaceAttachmentAccessor aa_hidden(grid, _a_hidden);

//	iterate through all subsets
//	each subset has its own display list
	Grid::edge_traits::secure_container	assEdges;

	for(int i = 0; i < sh.num_subsets(); ++i)
	{
	//	set up open-gl display lists
		GLuint display_list = p_obj->get_display_list(i);
		p_obj->set_display_list_mode(i, LGRM_DOUBLE_PASS_SHADED);
		glDeleteLists(display_list, 1);
		glNewList(display_list, GL_COMPILE);

		if((!render_all) && (!p_obj->subset_is_visible(i))){
			glEndList();
			continue;
		}

	//	draw triangles
		glBegin(GL_TRIANGLES);

		for(auto iter = sh.begin<Triangle>(i); iter != sh.end<Triangle>(i); ++iter)
		{
			Face* tri = *iter;
			//æ if(aaHidden[tri] && !renderAll)
			//æ 	continue;

			aa_rendered_face[tri] = true;

			vector3& n = aa_norm[tri];
			glNormal3f(n.x(), n.y(), n.z());

			for(int i = 0; i < 3; ++i)
			{
				aa_rendered_vrt[tri->vertex(i)] = true;
				vector3& v = aa_pos[tri->vertex(i)];
				glVertex3f(v.x(), v.y(), v.z());
			}

			// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(tri);
			// 	eiter != grid.associated_edges_end(tri); ++eiter)
			grid.associated_elements(assEdges, tri);
			for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
				aa_rendered_edge[assEdges[iedge]] = true;
		}

		for(auto iter = sh.begin<ConstrainingTriangle>(i);
			iter != sh.end<ConstrainingTriangle>(i); ++iter)
		{
			Face* tri = *iter;
			if(aa_hidden[tri] && !render_all)
				continue;

			aa_rendered_face[tri] = true;

			vector3& n = aa_norm[tri];
			glNormal3f(n.x(), n.y(), n.z());

			for(int i = 0; i < 3; ++i)
			{
				aa_rendered_vrt[tri->vertex(i)] = true;
				vector3& v = aa_pos[tri->vertex(i)];
				glVertex3f(v.x(), v.y(), v.z());
			}

			// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(tri);
			// 	eiter != grid.associated_edges_end(tri); ++eiter)
			// 	aaRenderedEDGE[*eiter] = true;
			grid.associated_elements(assEdges, tri);
			for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
				aa_rendered_edge[assEdges[iedge]] = true;
		}

		for(auto iter = sh.begin<ConstrainedTriangle>(i); iter != sh.end<ConstrainedTriangle>(i); ++iter)
		{
			Face* tri = *iter;
			if(aa_hidden[tri] && !render_all)
				continue;

			aa_rendered_face[tri] = true;

			vector3& n = aa_norm[tri];
			glNormal3f(n.x(), n.y(), n.z());

			for(int i = 0; i < 3; ++i)
			{
				aa_rendered_vrt[tri->vertex(i)] = true;
				vector3& v = aa_pos[tri->vertex(i)];
				glVertex3f(v.x(), v.y(), v.z());
			}

			// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(tri);
			// 	eiter != grid.associated_edges_end(tri); ++eiter)
			// 	aaRenderedEDGE[*eiter] = true;
			grid.associated_elements(assEdges, tri);
			for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
				aa_rendered_edge[assEdges[iedge]] = true;
		}

		glEnd();

	//	draw quads
		glBegin(GL_QUADS);

		for(QuadrilateralIterator iter = sh.begin<Quadrilateral>(i);
			iter != sh.end<Quadrilateral>(i); ++iter)
		{
			Quadrilateral* q = *iter;
			if(aa_hidden[q] && !render_all)
				continue;

			aa_rendered_face[q] = true;

			vector3& n = aa_norm[q];
			glNormal3f(n.x(), n.y(), n.z());

			for(int i = 0; i < 4; ++i)
			{
				aa_rendered_vrt[q->vertex(i)] = true;
				vector3& v = aa_pos[q->vertex(i)];
				glVertex3f(v.x(), v.y(), v.z());
			}

			// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(q);
			// 	eiter != grid.associated_edges_end(q); ++eiter)
			// 	aaRenderedEDGE[*eiter] = true;
			grid.associated_elements(assEdges, q);
			for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
				aa_rendered_edge[assEdges[iedge]] = true;
		}

		for(auto iter = sh.begin<ConstrainingQuadrilateral>(i); iter != sh.end<ConstrainingQuadrilateral>(i); ++iter)
		{
			ConstrainingQuadrilateral* q = *iter;
			if(aa_hidden[q] && !render_all)
				continue;

			aa_rendered_face[q] = true;

			vector3& n = aa_norm[q];
			glNormal3f(n.x(), n.y(), n.z());

			for(int i = 0; i < 4; ++i)
			{
				aa_rendered_vrt[q->vertex(i)] = true;
				vector3& v = aa_pos[q->vertex(i)];
				glVertex3f(v.x(), v.y(), v.z());
			}

			// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(q);
			// 	eiter != grid.associated_edges_end(q); ++eiter)
			// 	aaRenderedEDGE[*eiter] = true;
			grid.associated_elements(assEdges, q);
			for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
				aa_rendered_edge[assEdges[iedge]] = true;
		}

		for(auto iter = sh.begin<ConstrainedQuadrilateral>(i); iter != sh.end<ConstrainedQuadrilateral>(i); ++iter)
		{
			ConstrainedQuadrilateral* q = *iter;
			if(aa_hidden[q] && !render_all)
				continue;

			aa_rendered_face[q] = true;

			vector3& n = aa_norm[q];
			glNormal3f(n.x(), n.y(), n.z());

			for(int i = 0; i < 4; ++i)
			{
				aa_rendered_vrt[q->vertex(i)] = true;
				vector3& v = aa_pos[q->vertex(i)];
				glVertex3f(v.x(), v.y(), v.z());
			}

			// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(q);
			// 	eiter != grid.associated_edges_end(q); ++eiter)
			// 	aaRenderedEDGE[*eiter] = true;
			grid.associated_elements(assEdges, q);
			for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
				aa_rendered_edge[assEdges[iedge]] = true;
		}

		glEnd();
		glEndList();
	}
}

void LGScene::render_faces_without_clip_plane(LGObject* p_obj)
{
//	renders the faces of an object.
//	visibility is handled by the draw routine.
	render_faces(p_obj, p_obj->grid(), p_obj->subset_handler());
}

void LGScene::render_volumes(LGObject* p_obj)
{
	PROFILE_FUNC();
//	renders the volumes of an object.
//	clip planes are used.
	Grid& grid = p_obj->grid();
	SubsetHandler& sh = p_obj->subset_handler();

	Grid::VertexAttachmentAccessor aa_pos(grid, aPosition);
	Grid::FaceAttachmentAccessor aa_norm(grid, aNormal);
	Grid::FaceAttachmentAccessor	aa_sphere_face(grid, _a_sphere);
	Grid::VolumeAttachmentAccessor	aa_sphere_vol(grid, _a_sphere);

	Grid::FaceAttachmentAccessor aa_rendered_face(grid, _a_rendered);
	Grid::VolumeAttachmentAccessor aa_rendered_vol(grid, _a_rendered);
	Grid::VolumeAttachmentAccessor aa_hidden_vol(grid, _a_hidden);

//	preprocessing step:
//	sort all faces in a subset handler
	SubsetHandler& sh_face = p_obj->_sh_faces_for_vol_rendering;
	sh_face.clear();

	Grid::volume_traits::secure_container ass_vols;

//	too slow too!
//	iterate through all faces
	for(auto iter = grid.faces_begin(); iter != grid.faces_end(); ++iter)
	{
		Face* f = *iter;
	//	check whether the face is clipped.
		if(!clip_face(f, aa_sphere_face[f], aa_pos))
		{
		//	it's not.
		//	if it has exactly one visible adjacent volume, or no adjacent volumes at all,
		//	then it has to be displayed
			Volume* vis_vol = nullptr;
			int new_sub_ind = -1;

			int f_sub_ind = sh.get_subset_index(f);
			bool face_is_visible = (f_sub_ind != -1) && _draw_faces && p_obj->subset_is_visible(f_sub_ind);

			// Grid::AssociatedVolumeIterator volEnd = grid.associated_volumes_end(f);
			// Grid::AssociatedVolumeIterator volBegin = grid.associated_volumes_begin(f);
			grid.associated_elements(ass_vols, f);
			if(ass_vols.empty()){
			//	the face has to be rendered, since it is not adjacent to any volume
				if(face_is_visible)
					sh_face.assign_subset(f, f_sub_ind);
			}
			else{
				int num_vis_vols = 0;

				// for(Grid::AssociatedVolumeIterator vIter = volBegin;
				// 	vIter != volEnd; ++vIter)
				for(size_t ivol = 0; ivol < ass_vols.size(); ++ivol){
					Volume* ass_vol = ass_vols[ivol];
					if(aa_hidden_vol[ass_vol])
						continue;

					int v_sub_ind = sh.get_subset_index(ass_vol);

					if(v_sub_ind == -1)
						continue;

				//	check whether the subset of the volume is visible
					if(p_obj->subset_is_visible(v_sub_ind))
					{
					//	make sure the volume is not clipped.
					//	uncomment the following line for exact clipping tests.
						if(!clip_volume(ass_vol, aa_sphere_vol[ass_vol], aa_pos))
						{
							++num_vis_vols;
						//	if newSubInd has already been assigned, we'll reset it to 0
							if(new_sub_ind != -1){
								new_sub_ind = -1;
								vis_vol = nullptr;
							}
							else{
								new_sub_ind = v_sub_ind;
								vis_vol = ass_vol;
							}
						}
					}
				}

				//	if the face and volume subsets do not match, and if the face
				//	is visible, we'll simply draw the face itself.
				if((num_vis_vols < 2) && face_is_visible){
					sh_face.assign_subset(f, f_sub_ind);
					if(vis_vol)
						aa_rendered_vol[vis_vol] = true;
				}
				else if(new_sub_ind != -1){
				//	if newSubInd != -1 then the face has to be drawn.
					sh_face.assign_subset(f, new_sub_ind);
				//	mark the visible volume as rendered
					if(vis_vol){
						aa_rendered_vol[vis_vol] = true;
					}
				}
			}
		}
	}


//	make sure that shFace contains at as many subsets as the grids subset-handler
	if(sh_face.num_subsets() < sh.num_subsets())
		sh_face.set_subset_info(sh.num_subsets() - 1, SubsetInfo());

//	finally render the faces that we collected in the subset handler.
	render_faces(p_obj, grid, sh_face, true);
}

void LGScene::render_faces_with_clip_plane(LGObject* p_obj)
{
//	renders the faces of an object.
//	visibility is handled by the draw routine.
	Grid& grid = p_obj->grid();
	SubsetHandler& sh = p_obj->subset_handler();

	Grid::VertexAttachmentAccessor aa_pos(grid, aPosition);
	Grid::FaceAttachmentAccessor aa_norm(grid, aNormal);
	Grid::FaceAttachmentAccessor aa_sphere_face(grid, _a_sphere);
	Grid::VolumeAttachmentAccessor aa_sphere_vol(grid, _a_sphere);

	Grid::VertexAttachmentAccessor aa_rendered_vrt(grid, _a_rendered);
	Grid::EdgeAttachmentAccessor aa_rendered_edge(grid, _a_rendered);
	Grid::FaceAttachmentAccessor aa_rendered_face(grid, _a_rendered);

	Grid::FaceAttachmentAccessor aa_hidden(grid, _a_hidden);

	SetAttachmentValues(aa_rendered_vrt, grid.vertices_begin(), grid.vertices_end(), false);
	SetAttachmentValues(aa_rendered_edge, grid.edges_begin(), grid.edges_end(), false);
	SetAttachmentValues(aa_rendered_face, grid.faces_begin(), grid.faces_end(), false);

//	iterate through all subsets
//	each subset has its own display list
	Grid::edge_traits::secure_container	assEdges;

	for(int i = 0; i < sh.num_subsets(); ++i)
	{
	//	set up open-gl display lists
		GLuint display_list = p_obj->get_display_list(i);
		p_obj->set_display_list_mode(i, LGRM_DOUBLE_PASS_SHADED);
		glDeleteLists(display_list, 1);
		glNewList(display_list, GL_COMPILE);

		if(!p_obj->subset_is_visible(i)){
			glEndList();
			continue;
		}

	//	draw triangles
		if(sh.num<Triangle>(i) > 0 || sh.num<ConstrainingTriangle>(i) > 0 || sh.num<ConstrainedTriangle>(i) > 0)
		{
			glBegin(GL_TRIANGLES);

			for(auto iter = sh.begin<Triangle>(i); iter != sh.end<Triangle>(i); ++iter)
			{
				Triangle* tri = *iter;
				if(aa_hidden[tri])
					continue;

				if(!clip_face(tri, aa_sphere_face[tri], aa_pos))
				{
					aa_rendered_face[tri] = true;
					vector3& n = aa_norm[tri];
					glNormal3f(n.x(), n.y(), n.z());

					for(int j = 0; j < 3; ++j)
					{
						aa_rendered_vrt[tri->vertex(j)] = true;
						vector3& v = aa_pos[tri->vertex(j)];
						glVertex3f(v.x(), v.y(), v.z());
					}
					// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(tri);
					// 	eiter != grid.associated_edges_end(tri); ++eiter)
					// 		aaRenderedEDGE[*eiter] = true;
					grid.associated_elements(assEdges, tri);
					for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
						aa_rendered_edge[assEdges[iedge]] = true;
				}
			}

			for(auto iter = sh.begin<ConstrainingTriangle>(i); iter != sh.end<ConstrainingTriangle>(i); ++iter)
			{
				Face* tri = *iter;
				if(aa_hidden[tri])
					continue;

				if(!clip_face(tri, aa_sphere_face[tri], aa_pos))
				{
					aa_rendered_face[tri] = true;

					vector3& n = aa_norm[tri];
					glNormal3f(n.x(), n.y(), n.z());

					for(int i = 0; i < 3; ++i)
					{
						aa_rendered_vrt[tri->vertex(i)] = true;
						vector3& v = aa_pos[tri->vertex(i)];
						glVertex3f(v.x(), v.y(), v.z());
					}

					// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(tri);
					// 	eiter != grid.associated_edges_end(tri); ++eiter)
					// 		aaRenderedEDGE[*eiter] = true;
					grid.associated_elements(assEdges, tri);
					for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
						aa_rendered_edge[assEdges[iedge]] = true;
				}
			}

			for(auto iter = sh.begin<ConstrainedTriangle>(i); iter != sh.end<ConstrainedTriangle>(i); ++iter)
			{
				Face* tri = *iter;
				if(aa_hidden[tri])
					continue;

				if(!clip_face(tri, aa_sphere_face[tri], aa_pos))
				{
					aa_rendered_face[tri] = true;

					vector3& n = aa_norm[tri];
					glNormal3f(n.x(), n.y(), n.z());

					for(int i = 0; i < 3; ++i)
					{
						aa_rendered_vrt[tri->vertex(i)] = true;
						vector3& v = aa_pos[tri->vertex(i)];
						glVertex3f(v.x(), v.y(), v.z());
					}

					// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(tri);
					// 	eiter != grid.associated_edges_end(tri); ++eiter)
					// 		aaRenderedEDGE[*eiter] = true;
					grid.associated_elements(assEdges, tri);
					for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
						aa_rendered_edge[assEdges[iedge]] = true;
				}
			}

			glEnd();
		}

	//	draw quads
		if(sh.num<Quadrilateral>(i) > 0)
		{
			glBegin(GL_QUADS);

			for(auto iter = sh.begin<Quadrilateral>(i); iter != sh.end<Quadrilateral>(i); ++iter)
			{
				Face* q = *iter;
				if(aa_hidden[q])
					continue;

				if(!clip_face(q, aa_sphere_face[q], aa_pos))
				{
					aa_rendered_face[q] = true;
					vector3& n = aa_norm[q];
					glNormal3f(n.x(), n.y(), n.z());

					for(int j = 0; j < 4; ++j)
					{
						aa_rendered_vrt[q->vertex(j)] = true;
						vector3& v = aa_pos[q->vertex(j)];
						glVertex3f(v.x(), v.y(), v.z());
					}
					// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(q);
					// 	eiter != grid.associated_edges_end(q); ++eiter)
					// 		aaRenderedEDGE[*eiter] = true;
					grid.associated_elements(assEdges, q);
					for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
						aa_rendered_edge[assEdges[iedge]] = true;
				}
			}

			for(auto iter = sh.begin<ConstrainingQuadrilateral>(i); iter != sh.end<ConstrainingQuadrilateral>(i); ++iter)
			{
				Face* q = *iter;
				if(aa_hidden[q])
					continue;

				if(!clip_face(q, aa_sphere_face[q], aa_pos))
				{
					aa_rendered_face[q] = true;
					vector3& n = aa_norm[q];
					glNormal3f(n.x(), n.y(), n.z());

					for(int j = 0; j < 4; ++j)
					{
						aa_rendered_vrt[q->vertex(j)] = true;
						vector3& v = aa_pos[q->vertex(j)];
						glVertex3f(v.x(), v.y(), v.z());
					}

					// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(q);
					// 	eiter != grid.associated_edges_end(q); ++eiter)
					// 		aaRenderedEDGE[*eiter] = true;
					grid.associated_elements(assEdges, q);
					for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
						aa_rendered_edge[assEdges[iedge]] = true;
				}
			}

			for(auto iter = sh.begin<ConstrainedQuadrilateral>(i); iter != sh.end<ConstrainedQuadrilateral>(i); ++iter)
			{
				Face* q = *iter;
				if(aa_hidden[q])
					continue;

				if(!clip_face(q, aa_sphere_face[q], aa_pos))
				{
					aa_rendered_face[q] = true;
					vector3& n = aa_norm[q];
					glNormal3f(n.x(), n.y(), n.z());

					for(int j = 0; j < 4; ++j)
					{
						aa_rendered_vrt[q->vertex(j)] = true;
						vector3& v = aa_pos[q->vertex(j)];
						glVertex3f(v.x(), v.y(), v.z());
					}

					// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(q);
					// 	eiter != grid.associated_edges_end(q); ++eiter)
					// 		aaRenderedEDGE[*eiter] = true;
					grid.associated_elements(assEdges, q);
					for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
						aa_rendered_edge[assEdges[iedge]] = true;
				}
			}
			glEnd();
		}
		glEndList();
	}
}


Vertex* LGScene::
get_clicked_vertex(LGObject* obj, const vector3& from, const vector3& to)
{
	Vertex* vrtClosest = nullptr;

	if(obj){
		Grid& grid = obj->grid();
		Grid::VertexAttachmentAccessor aa_pos(grid, aPosition);
		Grid::VertexAttachmentAccessor aa_rendered_vrt(grid, _a_rendered);

	//	max distance - a safe overestimation
		number min_dist = _z_far * 2.;

		for(auto iter = grid.begin<Vertex>(); iter != grid.end<Vertex>(); ++iter)
		{
			Vertex* vrt = *iter;
			if(aa_rendered_vrt[vrt]){
				number t;
				number dist = DistancePointToLine(t, aa_pos[vrt], from, to);
				if(dist < min_dist && t > 0 && t < 1.2){
					vrtClosest = vrt;
					min_dist = dist;
				}
			}
		}
	}

	return vrtClosest;
}

Edge* LGScene::
get_clicked_edge(LGObject* obj, const vector3& from, const vector3& to, bool closestToTo)
{
	Edge* e_closest = nullptr;

	if(obj){
	//	iterate through the edges and check the center of each against
	//	the ray.
		Grid& grid = obj->grid();
		Grid::VertexAttachmentAccessor aa_pos(grid, aPosition);
		Grid::EdgeAttachmentAccessor aa_rendered_edge(grid, _a_rendered);

	//	max distance - a safe overestimation
		number min_dist = _z_far * 2.;
		vector3 dir;
		VecSubtract(dir, to, from);

		if(closestToTo){
		//	calculate the minimal distance of each edge to 'to'
			for(auto iter = grid.begin<Edge>(); iter != grid.end<Edge>(); ++iter)
			{
				Edge* e = *iter;
				if(!aa_rendered_edge[e])
					continue;
				number t;
				number dist = DistancePointToLine(t, to, aa_pos[e->vertex(0)],
													aa_pos[e->vertex(1)]);
				if(dist < min_dist && t > -0.2 && t < 1.2){
					e_closest = e;
					min_dist = dist;
				}
			}
		}
		else{
			number min_dist_sq = min_dist * min_dist;

		//	calculate the minimal distance of the center of each edge
		//	to the ray (from, to)
			for(auto iter = grid.begin<Edge>(); iter != grid.end<Edge>(); ++iter)
			{
				Edge* e = *iter;
				if(!aa_rendered_edge[e])
					continue;

			//todo:	make sure that at least one of the endpoints lies in front of the
			//	near-plane.
				vector3 isect_a, isect_b;
				LineLineIntersection3d(isect_a, isect_b,
									   aa_pos[e->vertex(0)], aa_pos[e->vertex(1)],
									   from, to);

			//	check whether isectA is in front of the near plane
				vector3 isect_dir;
				VecSubtract(isect_dir, isect_a, from);

				if((VecDot(isect_dir, dir) > 0)){
				//	distance between the two lines
					number distSq = VecDistanceSq(isect_a, isect_b);
					if(distSq < min_dist_sq){
						e_closest = e;
						min_dist_sq = distSq;
					}
				}
			}
		}
	}

	return e_closest;
}

Face* LGScene::
get_clicked_face(LGObject* p_obj, const vector3& from, const vector3& to)
{
	vector3 dir;
	VecSubtract(dir, to, from);

	Grid& grid = p_obj->grid();
//	SubsetHandler& sh = pObj->subset_handler();

	Grid::VertexAttachmentAccessor aa_pos(grid, aPosition);
	Grid::FaceAttachmentAccessor aa_norm(grid, aNormal);
	Grid::FaceAttachmentAccessor aa_sphere_face(grid, _a_sphere);

	Grid::FaceAttachmentAccessor aa_rendered_face(grid, _a_rendered);

	Face* clicked_face = nullptr;
	number max_dist_sq = _z_far * 2.;
	max_dist_sq *= max_dist_sq;

//	iterate through all faces
//	if the given ray cuts the triangles sphere, we'll examine it closer.
	for(auto iter = grid.faces_begin(); iter != grid.faces_end(); ++iter)
	{
		Face* f =*iter;

	//	make sure that the face is visible
		if(!aa_rendered_face[f])
			continue;

	//	check whether the subset is visible
	//	the si == -1 check makes sense, since faces that are in
	//	subset -1 can be drawn as side-faces of volumes and should
	//	thus be selectable. In face mode those faces are never drawn
	//	and thus aaRenderedFACE[f] == 0 holds.
//	this should no longer be required, since aaRenderedFACE already takes care of it.
//	The code leads to problems if faces were rendered as sides of volumes, even though
//	their subset was invisible.
/*
		int si = sh.get_subset_index(f);
		if(!(si == -1 || pObj->subset_is_visible(si)))
			continue;
*/
	//	check whether the face is invisible due to culling
		number norm_dot = VecDot(aa_norm[f], dir);
		if(!(_draw_mode_back & DM_SOLID)){
			if(norm_dot > 0)
				continue;
		}
		if(!(_draw_mode_front & DM_SOLID)){
			if(norm_dot < 0)
				continue;
		}

	//	check bounding sphere
		Sphere3& sphere = aa_sphere_face[f];
		number t;
		if(DistancePointToLine(t, sphere.get_center(), from, to) <= sphere.get_radius())
		{
			bool intersecting = false;
		//	perform line-face check
			vector3 v;
			number bc1, bc2, t;

			if(f->num_vertices() == 3){
				intersecting = RayTriangleIntersection(v, bc1, bc2, t,
													aa_pos[f->vertex(0)],
													aa_pos[f->vertex(1)],
													aa_pos[f->vertex(2)],
													from, dir);
			}
			else if(f->num_vertices() == 4)
			{
				intersecting = RayTriangleIntersection(v, bc1, bc2, t,
													aa_pos[f->vertex(0)],
													aa_pos[f->vertex(1)],
													aa_pos[f->vertex(2)],
													from, dir);
				if(!intersecting){
					intersecting = RayTriangleIntersection(v, bc1, bc2, t,
													aa_pos[f->vertex(0)],
													aa_pos[f->vertex(2)],
													aa_pos[f->vertex(3)],
													from, dir);
				}
			}

			if(intersecting)
			{
				if(t > 0){
					number dist_sq = VecDistanceSq(v, from);
					if(dist_sq < max_dist_sq){
					//	we found a face that's closer than the current one.
						clicked_face = f;
						max_dist_sq = dist_sq;
					}
				}
			}
		}
	}

	return clicked_face;
}

Volume* LGScene::
get_clicked_volume(LGObject* p_obj, const vector3& from, const vector3& to)
{
//	get the clicked face and check its associated volumes.
//	if a visible volume is associated, it is considered to be clicked.
	Grid& grid = p_obj->grid();
	SubsetHandler& sh = p_obj->subset_handler();
	Grid::VolumeAttachmentAccessor aa_rendered_vol(grid, _a_rendered);

//	get the clicked face.
	Face* clicked_face = get_clicked_face(p_obj, from, to);

//	if a face was clicked we'll check associated volumes
	if(clicked_face){
		vector<Volume*> v_vols;
		CollectVolumes(v_vols, grid, clicked_face);

		for(size_t i = 0; i < v_vols.size(); ++i){
			Volume* vol = v_vols[i];
			if(aa_rendered_vol[vol]){
				int si = sh.get_subset_index(vol);
				if(p_obj->subset_is_visible(si))
					return vol;
			}
		}
	}

//	nothing seems to be clicked
	return nullptr;
}

size_t LGScene::
get_vertices_in_rect(std::vector<Vertex*>& vrts_out,
					LGObject* obj,
					float x_min, float y_min, float x_max, float y_max)
{
//	iterate over all vertices and select them if they are visible
	vrts_out.clear();
	y_min = _view_height - y_min;
	y_max = _view_height - y_max;
	swap(y_min, y_max);

	if(obj)
	{
		GLdouble model_mat[16];
		GLdouble proj_mat[16];
		GLint viewport[4];

		glGetDoublev(GL_MODELVIEW_MATRIX, model_mat);
		glGetDoublev(GL_PROJECTION_MATRIX, proj_mat);
		glGetIntegerv(GL_VIEWPORT, viewport);

		GLdouble vx, vy, vz;

		Grid& grid = obj->grid();
		Grid::VertexAttachmentAccessor aa_pos(grid, aPosition);
		Grid::VertexAttachmentAccessor aa_rendered_vrt(grid, _a_rendered);

		Plane plane = near_clip_plane();
		for(VertexIterator iter = grid.begin<Vertex>(); iter != grid.end<Vertex>(); ++iter)
		{
			Vertex* vrt = *iter;
			if(aa_rendered_vrt[vrt]){
				vector3& pos = aa_pos[vrt];

				if(PlanePointTest(plane, pos) == RPI_INSIDE)
					continue;

				GLdouble px = pos.x();
				GLdouble py = pos.y();
				GLdouble pz = pos.z();

				gluProject(px, py, pz, model_mat, proj_mat, viewport, &vx, &vy, &vz);
				
				if(vx >= x_min && vx <= x_max && vy >= y_min && vy <= y_max && vz >= 0){
					vrts_out.push_back(vrt);
				}
			}
		}
	}

	return vrts_out.size();
}

size_t LGScene::
get_edges_in_rect(std::vector<Edge*>& edges_out,
					LGObject* obj,
					float x_min, float y_min, float x_max, float y_max)
{
	edges_out.clear();
	y_min = _view_height - y_min;
	y_max = _view_height - y_max;
	swap(y_min, y_max);

	if(obj)
	{
		GLdouble model_mat[16];
		GLdouble proj_mat[16];
		GLint viewport[4];

		glGetDoublev(GL_MODELVIEW_MATRIX, model_mat);
		glGetDoublev(GL_PROJECTION_MATRIX, proj_mat);
		glGetIntegerv(GL_VIEWPORT, viewport);

		GLdouble vx, vy, vz;
		Grid& grid = obj->grid();
		Grid::VertexAttachmentAccessor aa_pos(grid, aPosition);
		Grid::EdgeAttachmentAccessor aa_rendered_edge(grid, _a_rendered);
		Plane plane = near_clip_plane();

		for(auto iter = grid.begin<Edge>(); iter != grid.end<Edge>(); ++iter)
		{
			Edge* e = *iter;
			if(!aa_rendered_edge[e])
				continue;

			bool all_in = true;
			for(size_t i = 0; i < e->num_vertices(); ++i){
				Vertex* vrt = e->vertex(i);
				vector3& pos = aa_pos[vrt];

				if(PlanePointTest(plane, pos) == RPI_INSIDE){
					all_in = false;
					break;
				}

				GLdouble px = pos.x();
				GLdouble py = pos.y();
				GLdouble pz = pos.z();

				gluProject(px, py, pz, model_mat, proj_mat, viewport, &vx, &vy, &vz);

				if(vx < x_min || vx > x_max || vy < y_min || vy > y_max || vz < 0){
					all_in = false;
					break;
				}
			}
			if(all_in)
				edges_out.push_back(e);
		}
	}

	return edges_out.size();
}

size_t LGScene::
get_faces_in_rect(std::vector<Face*>& faces_out,
					LGObject* obj,
					float x_min, float y_min, float x_max, float y_max)
{
	faces_out.clear();
	y_min = _view_height - y_min;
	y_max = _view_height - y_max;
	swap(y_min, y_max);

	if(obj)
	{
		GLdouble model_mat[16];
		GLdouble proj_mat[16];
		GLint viewport[4];

		glGetDoublev(GL_MODELVIEW_MATRIX, model_mat);
		glGetDoublev(GL_PROJECTION_MATRIX, proj_mat);
		glGetIntegerv(GL_VIEWPORT, viewport);

		GLdouble vx, vy, vz;
		Grid& grid = obj->grid();
		Grid::VertexAttachmentAccessor aa_pos(grid, aPosition);
		Grid::FaceAttachmentAccessor aa_rendered_face(grid, _a_rendered);
		Plane plane = near_clip_plane();

		for(FaceIterator iter = grid.begin<Face>(); iter != grid.end<Face>(); ++iter)
		{
			Face* f = *iter;
			if(!aa_rendered_face[f])
				continue;

			bool allIn = true;
			for(size_t i = 0; i < f->num_vertices(); ++i){
				Vertex* vrt = f->vertex(i);
				vector3& pos = aa_pos[vrt];
				if(PlanePointTest(plane, pos) == RPI_INSIDE){
					allIn = false;
					break;
				}

				GLdouble px = pos.x();
				GLdouble py = pos.y();
				GLdouble pz = pos.z();

				gluProject(px, py, pz, model_mat, proj_mat, viewport, &vx, &vy, &vz);

				if(vx < x_min || vx > x_max || vy < y_min || vy > y_max || vz < 0){
					allIn = false;
					break;
				}
			}
			if(allIn)
				faces_out.push_back(f);
		}
	}

	return faces_out.size();
}

size_t LGScene::
get_volumes_in_rect(std::vector<Volume*>& vols_out,
					LGObject* obj,
					float x_min, float y_min, float x_max, float y_max)
{
	vols_out.clear();
	y_min = _view_height - y_min;
	y_max = _view_height - y_max;
	swap(y_min, y_max);

	if(obj)
	{
		GLdouble model_mat[16];
		GLdouble proj_mat[16];
		GLint viewport[4];

		glGetDoublev(GL_MODELVIEW_MATRIX, model_mat);
		glGetDoublev(GL_PROJECTION_MATRIX, proj_mat);
		glGetIntegerv(GL_VIEWPORT, viewport);

		GLdouble vx, vy, vz;
		Grid& grid = obj->grid();
		Grid::VertexAttachmentAccessor aa_pos(grid, aPosition);
		Grid::VolumeAttachmentAccessor aa_rendered_vol(grid, _a_rendered);
		Plane plane = near_clip_plane();

		for(VolumeIterator iter = grid.begin<Volume>();
			iter != grid.end<Volume>(); ++iter)
		{
			Volume* v = *iter;

		//	aaRendered is bad here, since only outer volumes are rendered...
			//if(!aaRenderedVOL[v])
			//	continue;
		//	todo: add an is_visible(v) method.
			if(!obj->subset_is_visible(obj->subset_handler().get_subset_index(v)))
				continue;

			bool allIn = true;
			for(size_t i = 0; i < v->num_vertices(); ++i){
				Vertex* vrt = v->vertex(i);
				vector3& pos = aa_pos[vrt];
				if(PlanePointTest(plane, pos) == RPI_INSIDE){
					allIn = false;
					break;
				}

				GLdouble px = pos.x();
				GLdouble py = pos.y();
				GLdouble pz = pos.z();

				gluProject(px, py, pz, model_mat, proj_mat, viewport, &vx, &vy, &vz);

				if(vx < x_min || vx > x_max || vy < y_min || vy > y_max || vz < 0){
					allIn = false;
					break;
				}
			}
			if(allIn)
				vols_out.push_back(v);
		}
	}

	return vols_out.size();
}

size_t LGScene::
get_edges_in_rect_cut(std::vector<Edge*>& edges_out,
					LGObject* obj,
					float x_min, float y_min, float x_max, float y_max)
{
	edges_out.clear();
	y_min = _view_height - y_min;
	y_max = _view_height - y_max;
	swap(y_min, y_max);

	vector3 box_min(x_min, y_min, 0);
	vector3 box_max(x_max, y_max, 1.);

	if(obj)
	{
		GLdouble model_mat[16];
		GLdouble proj_mat[16];
		GLint viewport[4];

		glGetDoublev(GL_MODELVIEW_MATRIX, model_mat);
		glGetDoublev(GL_PROJECTION_MATRIX, proj_mat);
		glGetIntegerv(GL_VIEWPORT, viewport);

		GLdouble vx1, vy1, vz1;
		GLdouble vx2, vy2, vz2;

		Grid& grid = obj->grid();
		Grid::VertexAttachmentAccessor aaPos(grid, aPosition);
		Grid::EdgeAttachmentAccessor aa_rendered_edge(grid, _a_rendered);
		Plane plane = near_clip_plane();

		for(EdgeIterator iter = grid.begin<Edge>();
			iter != grid.end<Edge>(); ++iter)
		{
			Edge* e = *iter;
			if(!aa_rendered_edge[e])
				continue;

			Vertex* vrt_1 = e->vertex(0);
			Vertex* vrt_2 = e->vertex(1);
			vector3& pos_1 = aaPos[vrt_1];
			vector3& pos_2 = aaPos[vrt_2];

		//	at least one of the vertices has to lie in front of the clip plane
			if((PlanePointTest(plane, pos_1) == RPI_INSIDE) && (PlanePointTest(plane, pos_2) == RPI_INSIDE))
			{
				continue;
			}

			GLdouble px1 = pos_1.x();
			GLdouble py1 = pos_1.y();
			GLdouble pz1 = pos_1.z();
			GLdouble px2 = pos_2.x();
			GLdouble py2 = pos_2.y();
			GLdouble pz2 = pos_2.z();

			gluProject(px1, py1, pz1, model_mat, proj_mat, viewport, &vx1, &vy1, &vz1);
			gluProject(px2, py2, pz2, model_mat, proj_mat, viewport, &vx2, &vy2, &vz2);


			if(LineBoxIntersection(vector3(vx1, vy1, vz1),
								   vector3(vx2, vy2, vz2),
								   box_min, box_max))
			{
				edges_out.push_back(e);
			}
		}
	}

	return edges_out.size();
}

size_t LGScene::
get_faces_in_rect_cut(std::vector<Face*>& faces_out,
				  LGObject* obj,
				  float x_min, float y_min, float x_max, float y_max)
{
	faces_out.clear();
	y_min = _view_height - y_min;
	y_max = _view_height - y_max;
	swap(y_min, y_max);

	vector3 boxMin(x_min, y_min, 0);
	vector3 boxMax(x_max, y_max, 1.);

	if(obj)
	{
		GLdouble model_mat[16];
		GLdouble proj_mat[16];
		GLint viewport[4];

		glGetDoublev(GL_MODELVIEW_MATRIX, model_mat);
		glGetDoublev(GL_PROJECTION_MATRIX, proj_mat);
		glGetIntegerv(GL_VIEWPORT, viewport);

		Grid& grid = obj->grid();
		SubsetHandler& sh = obj->subset_handler();
		Grid::VertexAttachmentAccessor aa_pos(grid, aPosition);
		Grid::FaceAttachmentAccessor aa_rendered_face(grid, _a_rendered);
		Plane plane = near_clip_plane();

		for(int si = 0; si < sh.num_subsets(); ++si)
		{
			if(!obj->subset_is_visible(si))
				continue;

			for(auto iter = sh.begin<Face>(si); iter != sh.end<Face>(si); ++iter)
			{
				Face* f = *iter;
				if(!aa_rendered_face[f])
					continue;

				assert((f->num_vertices() == 3 || f->num_vertices() == 4) && "unsupported number of vertices");

				vector3 proj_pos[4];
				bool one_lies_in_front = false;

				for(size_t i = 0; i < f->num_vertices(); ++i){
					Vertex* vrt = f->vertex(i);
					vector3& pos = aa_pos[vrt];

				//	at least one of the vertices has to lie in front of the clip plane
					if(PlanePointTest(plane, pos) == RPI_OUTSIDE)
						one_lies_in_front = true;

					GLdouble px = pos.x();
					GLdouble py = pos.y();
					GLdouble pz = pos.z();
					GLdouble vx, vy, vz;

					gluProject(px, py, pz, model_mat, proj_mat, viewport, &vx, &vy, &vz);

					proj_pos[i].x() = vx;
					proj_pos[i].y() = vy;
					proj_pos[i].z() = vz;
				}

				if(!one_lies_in_front)
					continue;

				bool intersecting = TriangleBoxIntersection(
										proj_pos[0], proj_pos[1], proj_pos[2],
										boxMin, boxMax);

				if(!intersecting && f->num_vertices() == 4){
					intersecting = TriangleBoxIntersection(
										proj_pos[0], proj_pos[2], proj_pos[3],
										boxMin, boxMax);
				}

				if(intersecting)
					faces_out.push_back(f);
			}
		}
	}

	return faces_out.size();
}

size_t LGScene::
get_volumes_in_rect_cut(std::vector<Volume*>& vols_out,
				  LGObject* obj,
				  float x_min, float y_min, float x_max, float y_max)
{
	vols_out.clear();
	y_min = _view_height - y_min;
	y_max = _view_height - y_max;
	swap(y_min, y_max);

	vector3 box_min(x_min, y_min, 0);
	vector3 box_max(x_max, y_max, 1.);

	if(obj)
	{
		GLdouble model_mat[16];
		GLdouble proj_mat[16];
		GLint viewport[4];

		glGetDoublev(GL_MODELVIEW_MATRIX, model_mat);
		glGetDoublev(GL_PROJECTION_MATRIX, proj_mat);
		glGetIntegerv(GL_VIEWPORT, viewport);

		Grid& grid = obj->grid();
		SubsetHandler& sh = obj->subset_handler();
		Grid::VertexAttachmentAccessor aaPos(grid, aPosition);
		Plane plane = near_clip_plane();

		grid.begin_marking();

		Grid::volume_traits::secure_container vols;

		for(auto iter = grid.begin<Face>(); iter != grid.end<Face>(); ++iter)
		{
			Face* f = *iter;

			assert((f->num_vertices() == 3 || f->num_vertices() == 4) && "unsupported number of vertices");

			vector3 projPos[4];
			bool one_lies_in_front = false;
			for(size_t i = 0; i < f->num_vertices(); ++i){
				Vertex* vrt = f->vertex(i);
				vector3& pos = aaPos[vrt];
			//	at least one of the vertices has to lie in front of the clip plane
				if(PlanePointTest(plane, pos) == RPI_OUTSIDE)
					one_lies_in_front = true;

				GLdouble px = pos.x();
				GLdouble py = pos.y();
				GLdouble pz = pos.z();
				GLdouble vx, vy, vz;

				gluProject(px, py, pz, model_mat, proj_mat, viewport, &vx, &vy, &vz);

				projPos[i].x() = vx;
				projPos[i].y() = vy;
				projPos[i].z() = vz;
			}

			if(!one_lies_in_front)
				continue;

			bool intersecting = TriangleBoxIntersection(
									projPos[0], projPos[1], projPos[2],
									box_min, box_max);

			if(!intersecting && f->num_vertices() == 4){
				intersecting = TriangleBoxIntersection(
									projPos[0], projPos[2], projPos[3],
									box_min, box_max);
			}

			if(intersecting){
			//	since the face is intersecting, associated volumes do so too.
				grid.associated_elements(vols, f);
				for(size_t ivol = 0; ivol < vols.size(); ++ivol){
					Volume* vol = vols[ivol];
					if(!grid.is_marked(vol)){
						grid.mark(vol);
						if(obj->subset_is_visible(sh.get_subset_index(vol)))
							vols_out.push_back(vol);
					}
				}
			}
		}
		grid.end_marking();
	}

	return vols_out.size();
}


void LGScene::
unhide_elements(LGObject* obj)
{
	using namespace ug;
	unhide_elements<Vertex>(obj);
	unhide_elements<Edge>(obj);
	unhide_elements<Face>(obj);
	unhide_elements<Volume>(obj);
}

void LGScene::
set_element_draw_mode(bool draw_vrts, bool draw_edges, bool draw_faces, bool draw_vols)
{
	_draw_vertices = draw_vrts;
	_draw_edges = draw_edges;
	_draw_faces = draw_faces;
	_draw_volumes = draw_vols;
}
