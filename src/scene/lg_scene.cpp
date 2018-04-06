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
#include "lg_scene.h"
#include "gl_includes.h"

using namespace std;
using namespace ug;

LGScene::LGScene() :
	m_camFrom(0, 0, 0),
	m_camDir(0, 0, -1),
	m_camUp(0, 1, 0),
	m_worldScale(1, 1, 1),
	m_aHidden(true),
	m_drawVertices(true),
	m_drawEdges(true),
	m_drawFaces(true),
	m_drawVolumes(true)
{
	m_drawModeFront = m_drawModeBack = DM_SOLID_WIRE;

	for(int i = 0; i < numClipPlanes(); ++i)
	{
		m_clipPlaneEnabled[i] = false;
	}
}

void LGScene::set_draw_mode_front(unsigned int drawMode)
{
	m_drawModeFront = drawMode;
	emit visuals_updated();
}

ug::Plane LGScene::near_clip_plane()
{
	vector3 v;
	VecNormalize(v, m_camDir);
	VecScale(v, v, m_zNear);
	VecAdd(v, v, m_camFrom);
	return Plane(m_camFrom, m_camDir);
}

void LGScene::set_draw_mode_back(unsigned int drawMode)
{
	m_drawModeBack = drawMode;
	emit visuals_updated();
}

void LGScene::set_transform(float* mat)
{
	memcpy(m_matTransform, mat, sizeof(float)*16);
}

void LGScene::set_camera_parameters(float fromX, float fromY, float fromZ,
								   float dirX, float dirY, float dirZ,
								   float upX, float upY, float upZ)
{
	m_camFrom = vector3(fromX, fromY, fromZ);
	m_camDir = vector3(dirX, dirY, dirZ);
	m_camUp = vector3(upX, upY, upZ);
}

void LGScene::set_world_scale(float x, float y, float z)
{
	m_worldScale = vector3(x, y, z);
}

void LGScene::set_perspective(float fovy, int viewWidth, int viewHeight,
							  float zNear, float zFar)
{
	m_viewWidth = viewWidth;
	m_viewHeight = viewHeight;
	m_fovy = fovy;
	m_aspectRatio = float(viewWidth) / viewHeight;
	m_zNear = zNear;
	m_zFar = zFar;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(m_fovy, m_aspectRatio, m_zNear, m_zFar);
	glMatrixMode(GL_MODELVIEW);
}

void LGScene::set_ortho_perspective(float left, float right, float bottom,
									float top, float zNear, float zFar)
{
	m_viewWidth = right-left;
	m_viewHeight = top-bottom;
	m_zNear = zNear;
	m_zFar = zFar;
	m_fovy = 1;
	m_aspectRatio = float(m_viewWidth) / m_viewHeight;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(left, right, bottom, top, zNear, zFar);
	glMatrixMode(GL_MODELVIEW);
}


int LGScene::add_object(LGObject* obj, bool autoDelete)
{
	if(!obj->grid().has_face_attachment(m_aSphere))
		obj->grid().attach_to_faces(m_aSphere);
	if(!obj->grid().has_volume_attachment(m_aSphere))
		obj->grid().attach_to_volumes(m_aSphere);
	if(!obj->grid().has_vertex_attachment(m_aRendered))
	{
		obj->grid().attach_to_vertices(m_aRendered);
		obj->grid().attach_to_edges(m_aRendered);
		obj->grid().attach_to_faces(m_aRendered);
		obj->grid().attach_to_volumes(m_aRendered);
	}
	if(!obj->grid().has_vertex_attachment(m_aHidden)){
		obj->grid().attach_to_vertices(m_aHidden);
		obj->grid().attach_to_edges(m_aHidden);
		obj->grid().attach_to_faces(m_aHidden);
		obj->grid().attach_to_volumes(m_aHidden);
	}

	connect(obj, SIGNAL(sig_geometry_changed()), this, SLOT(object_geometry_changed()));
	connect(obj, SIGNAL(sig_visuals_changed()), this, SLOT(object_visuals_changed()));
	connect(obj, SIGNAL(sig_selection_changed()), this, SLOT(object_selection_changed()));
	connect(obj, SIGNAL(sig_properties_changed()), this, SLOT(object_properties_changed()));

	calculate_bounding_spheres(obj);
	int retVal = BaseClass::add_object(obj, autoDelete);
	update_visuals(obj);

	emit geometry_changed();

	return retVal;
}

void LGScene::visibility_changed(ISceneObject* pObj)
{
//	update the geometry if volumes are contained

	if(LGObject* lgObj = dynamic_cast<LGObject*>(pObj))
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
	LGObject* obj = dynamic_cast<LGObject*>(sender());
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
	LGObject* obj = dynamic_cast<LGObject*>(sender());
	if(obj){
		update_visuals(obj);
	}
}

void LGScene::object_selection_changed()
{
	LGObject* obj = dynamic_cast<LGObject*>(sender());
	if(obj){
		update_selection_visuals(obj);
		emit selection_changed();
	}
}

void LGScene::object_properties_changed()
{
	if(ISceneObject* obj = dynamic_cast<ISceneObject*>(sender())){
		emit IScene::object_properties_changed(obj);
	}
}

void LGScene::get_bounding_box(ug::vector3& vMinOut, ug::vector3& vMaxOut)
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
		vMinOut[i] *= m_worldScale[i];
		vMaxOut[i] *= m_worldScale[i];
	}
}

ug::Sphere3 LGScene::get_bounding_sphere()
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
	m_clipPlaneEnabled[index] = enable;
}

//	index from 0 to 2: xy, xz, yz
//	values from 0 to 1.
void LGScene::setClipPlane(int index, const ug::Plane& plane)
{
	m_clipPlanes[index] = plane;
}

void LGScene::calculate_bounding_spheres(LGObject* pObj)
{
	Grid& grid = pObj->grid();

//	calculate bounding-spheres.
	Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);
	Grid::FaceAttachmentAccessor<ASphere>	aaSphereFACE(grid, m_aSphere);
	Grid::VolumeAttachmentAccessor<ASphere>	aaSphereVOL(grid, m_aSphere);

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
			if(PlanePointTest(m_clipPlanes[i], aaPos[v]) == RPI_OUTSIDE)
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
			if(ClipEdge(e, m_clipPlanes[i], aaPos))
				return true;
		}
	}

	return false;
}

bool LGScene::clip_face(Face* f, const ug::Sphere3& boundingSphere,
						Grid::VertexAttachmentAccessor<APosition>& aaPos)
{
//	exact version
	for(int i = 0; i < numClipPlanes(); ++i){
		if(clipPlaneIsEnabled(i)){
			if(ClipFace(f, boundingSphere, m_clipPlanes[i], aaPos))
				return true;
		}
	}

/*	does not yet work correctly
//	fast version: simply check the center of the bounding-sphere
	for(int i = 0; i < numClipPlanes(); ++i)
	{
		if(clipPlaneIsEnabled(i))
		{
			if(PlanePointTest(m_clipPlanes[i], boundingSphere.get_center()) ==
				RPI_OUTSIDE)
				return true;
		}
	}
*/
	return false;
}

bool LGScene::clip_volume(Volume* v, const ug::Sphere3& boundingSphere,
						Grid::VertexAttachmentAccessor<APosition>& aaPos)
{
//	exact version
	for(int i = 0; i < numClipPlanes(); ++i){
		if(clipPlaneIsEnabled(i)){
			if(ClipVolume(v, boundingSphere, m_clipPlanes[i], aaPos))
				return true;
		}
	}
/*	does not yet work correctly
//	fast version: simply check the center of the bounding-sphere
	for(int i = 0; i < numClipPlanes(); ++i)
	{
		if(clipPlaneIsEnabled(i))
		{
			if(PlanePointTest(m_clipPlanes[i], boundingSphere.get_center()) ==
				RPI_OUTSIDE)
				return true;
		}
	}
*/
	return false;
}

RelativePositionIndicator LGScene::clip_sphere(const Sphere3& sphere)
{
	RelativePositionIndicator retVal = RPI_INSIDE;
	for(int i = 0; i < numClipPlanes(); ++i)
	{
		if(clipPlaneIsEnabled(i))
		{
			RelativePositionIndicator nVal = PlaneSphereTest(m_clipPlanes[i], sphere);
			if(nVal > retVal)
				retVal = nVal;
			if(retVal == RPI_OUTSIDE)
				return retVal;
		}
	}
	return retVal;
}

RelativePositionIndicator LGScene::clip_point(const ug::vector3& point)
{
	RelativePositionIndicator retVal = RPI_INSIDE;
	for(int i = 0; i < numClipPlanes(); ++i)
	{
		if(clipPlaneIsEnabled(i))
		{
			RelativePositionIndicator nVal = PlanePointTest(m_clipPlanes[i], point);
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
					nearOut = (float) PlanePointDistance(m_clipPlanes[i], from) * 0.1;
				else
				{
					float tDist = (float) PlanePointDistance(m_clipPlanes[i], from) * 0.1;
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
				drawDoublePassShaded |= (obj->get_display_list_mode(j) == LGRM_DOUBLE_PASS_SHADED);
				drawSinglePassColor |= (obj->get_display_list_mode(j) == LGRM_SINGLE_PASS_COLOR);
				drawDoublePassColor |= (obj->get_display_list_mode(j) == LGRM_DOUBLE_PASS_COLOR);
				drawSinglePassNoLight |= (obj->get_display_list_mode(j) == LGRM_SINGLE_PASS_NO_LIGHT);
			}

		//	draw double-pass-shaded
			if(drawDoublePassShaded){
				glPolygonOffset(1, 2);
				int drawMode[2];
				drawMode[0] = m_drawModeFront;
				drawMode[1] = m_drawModeBack;

			//	we'll do this twice. for both orientations.
			//	draw back first. wire-frame is rendered without z-buffer-write.
				for(int iPass = 1; iPass >= 0; --iPass)
				{
				//	if the draw-mode is set to DM_NONE we'll continue right away
					if(drawMode[iPass] == DM_NONE)
						continue;

				//	init world-matrix
					glMatrixMode(GL_MODELVIEW);
					glLoadIdentity();

					if(iPass == 0)
					{
					//	normal orientation
						glLightfv( GL_LIGHT0, GL_POSITION, lightDirection);
						glLightfv( GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
						glCullFace(GL_BACK);
					}
					else
					{
					//	inverted orientation
						glLightfv( GL_LIGHT0, GL_POSITION, lightDirectionInv);
						glLightfv( GL_LIGHT0, GL_DIFFUSE, lightDiffuseInv);
						glCullFace(GL_FRONT);
					}

					glMultMatrixf(m_matTransform);

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
//TODO:	add display-list visibility
					if((obj->get_display_list_mode(j) == LGRM_SINGLE_PASS_COLOR) ||
					   (obj->get_display_list_mode(j) == LGRM_DOUBLE_PASS_COLOR))
					{
						glMatrixMode(GL_MODELVIEW);
						glLoadIdentity();
						glMultMatrixf(m_matTransform);

						glLineWidth(2.f);
						//glDisable(GL_BLEND);
						glEnable(GL_BLEND);
						glEnable(GL_LINE_SMOOTH);
						glDisable(GL_LIGHTING);
						//glDisable(GL_POLYGON_OFFSET_FILL);
						glPolygonOffset(1, 1);
						glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

						if(drawDoublePassColor)
							glDisable(GL_CULL_FACE);
						else
							glEnable(GL_CULL_FACE);

						glCallList(obj->get_display_list(j));

						glEnable(GL_POLYGON_OFFSET_FILL);
						glEnable(GL_LIGHTING);
					}
				}
			}


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
						glMultMatrixf(m_matTransform);

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
	LGObject* pLGObj = dynamic_cast<LGObject*>(pObj);
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
	Grid::VertexAttachmentAccessor<ABool>	aaRenderedVRT(grid, m_aRendered);
	Grid::EdgeAttachmentAccessor<ABool>		aaRenderedEDGE(grid, m_aRendered);
	Grid::FaceAttachmentAccessor<ABool>		aaRenderedFACE(grid, m_aRendered);
	Grid::VolumeAttachmentAccessor<ABool>	aaRenderedVOL(grid, m_aRendered);

	SetAttachmentValues(aaRenderedVRT, grid.vertices_begin(),
						grid.vertices_end(), false);
	SetAttachmentValues(aaRenderedEDGE, grid.edges_begin(),
						grid.edges_end(), false);
	SetAttachmentValues(aaRenderedFACE, grid.faces_begin(),
						grid.faces_end(), false);
	SetAttachmentValues(aaRenderedVOL, grid.volumes_begin(),
						grid.volumes_end(), false);

//	calculate the number of required display lists
	int numSubsets = pObj->subset_handler().num_subsets();
	int numDisplayLists = 0;

	bool drawVolumes	= (m_drawVolumes && (pObj->grid().num_volumes() > 0));
	bool drawFaces		= (m_drawFaces && (pObj->grid().num_faces() > 0) && (!drawVolumes));
	bool drawEdges		= (m_drawEdges && (pObj->grid().num_edges() > 0));
	bool drawVertices 	= (m_drawVertices && (pObj->grid().num_vertices() > 0));

/*
	if((pObj->volume_rendering_enabled()))
		numDisplayLists += numSubsets;
	else if((pObj->face_rendering_enabled()))
		numDisplayLists += numSubsets;
	if(pObj->edge_rendering_enabled())
		numDisplayLists += numSubsets;
	if(pObj->vertex_rendering_enabled())
		numDisplayLists += numSubsets;
*/
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
		pObj->m_selectionDisplayListIndex = curDisplayListIndex;
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
	if(obj->m_selectionDisplayListIndex < 0)
		update_visuals(obj);
	else{
		render_selection(obj, obj->m_selectionDisplayListIndex);
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
	render_edges(pObj, vector4(0.1f, 1.f, 0.1f, 1.f),
				 grid.edges_begin(), grid.edges_end(), aaPos);

//	draw points
	render_points(pObj, vector4(0.1f, 0.1f, 1.f, 1.f),
				 grid.vertices_begin(), grid.vertices_end(), aaPos);

	glEndList();
}

void LGScene::render_creases(LGObject* pObj, int displayListIndex)
{
	Grid& grid = pObj->grid();
	SubsetHandler& sh = pObj->crease_handler();
	Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);

	GLuint displayList = pObj->get_display_list(displayListIndex);
	pObj->set_display_list_mode(displayListIndex, LGRM_SINGLE_PASS_COLOR);
	glDeleteLists(displayList, 1);
	glNewList(displayList, GL_COMPILE);

//	draw vertices
	render_points(pObj, vector4(0.1f, 0.1f, 0.9f, 1.f),
				 sh.begin<Vertex>(REM_FIXED), sh.end<Vertex>(REM_FIXED), aaPos);
//	draw edges
	render_edges(pObj, vector4(0.1f, 0.1f, 0.9f, 1.f),
				 sh.begin<Edge>(REM_CREASE), sh.end<Edge>(REM_CREASE), aaPos);

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

	bool drawVolumes	= (m_drawVolumes && (pObj->grid().num_volumes() > 0));
	bool drawFaces		= (m_drawFaces && (pObj->grid().num_faces() > 0) && (!drawVolumes));
	bool drawEdges		= (m_drawEdges && (pObj->grid().num_edges() > 0));
	bool drawVertices 	= (m_drawVertices && (pObj->grid().num_vertices() > 0));

//	draw faces
	//if(pObj->face_rendering_enabled()){
	if(drawFaces || drawVolumes){
		render_triangles(pObj, faceColor, sel.begin<Triangle>(),
						sel.end<Triangle>(), aaPos, aaNorm);

		if(sel.num<ConstrainedTriangle>() > 0)
			render_triangles(pObj, faceColor, sel.begin<ConstrainedTriangle>(),
							sel.end<ConstrainedTriangle>(), aaPos, aaNorm);

		if(sel.num<ConstrainingTriangle>() > 0)
			render_triangles(pObj, faceColor, sel.begin<ConstrainingTriangle>(),
							sel.end<ConstrainingTriangle>(), aaPos, aaNorm);

		render_quadrilaterals(pObj, faceColor, sel.begin<Quadrilateral>(),
							  sel.end<Quadrilateral>(), aaPos, aaNorm);

		if(sel.num<ConstrainedQuadrilateral>() > 0)
			render_quadrilaterals(pObj, faceColor, sel.begin<ConstrainedQuadrilateral>(),
								  sel.end<ConstrainedQuadrilateral>(), aaPos, aaNorm);

		if(sel.num<ConstrainingQuadrilateral>() > 0)
			render_quadrilaterals(pObj, faceColor, sel.begin<ConstrainingQuadrilateral>(),
								  sel.end<ConstrainingQuadrilateral>(), aaPos, aaNorm);
	}

//	draw volumes
//	draw volumes after faces, since the draw-order is important.
	//if(pObj->volume_rendering_enabled()){
	if(drawVolumes){
		rerender_volumes(pObj, volColor, sel.begin<Volume>(),
						 sel.end<Volume>(), aaPos, aaNorm);
	}

//	draw edges
	if(drawEdges || drawFaces || drawVolumes){
		render_edges(pObj, edgeColor,
					 sel.begin<Edge>(), sel.end<Edge>(), aaPos);
	}

//	draw points
	if(drawVertices || drawEdges || drawFaces || drawVolumes){
		render_points(pObj, vrtColor,
					 sel.begin<Vertex>(), sel.end<Vertex>(), aaPos);
	}

	glEndList();
}

void LGScene::render_points(LGObject* pObj, const ug::vector4& color,
							  ug::VertexIterator vrtsBegin,
							  ug::VertexIterator vrtsEnd,
							  Grid::VertexAttachmentAccessor<APosition>& aaPos)
{
	Grid::VertexAttachmentAccessor<ABool> aaRenderedVRT(pObj->grid(), m_aRendered);

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
	Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);
	Grid::VertexAttachmentAccessor<ABool> aaRenderedVRT(grid, m_aRendered);
	Grid::VertexAttachmentAccessor<ABool> aaHiddenVRT(grid, m_aHidden);

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

void LGScene::render_edges(LGObject* pObj, const ug::vector4& color,
						  ug::EdgeIterator edgesBegin,
						  ug::EdgeIterator edgesEnd,
						  Grid::VertexAttachmentAccessor<APosition>& aaPos)
{
	Grid& grid = pObj->grid();
	Grid::EdgeAttachmentAccessor<ABool> aaRenderedEDGE(grid, m_aRendered);

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
	Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);
	Grid::VertexAttachmentAccessor<ABool> aaRenderedVRT(grid, m_aRendered);
	Grid::EdgeAttachmentAccessor<ABool> aaRenderedEDGE(grid, m_aRendered);
	Grid::EdgeAttachmentAccessor<ABool> aaHiddenEDGE(grid, m_aHidden);

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

void LGScene::render_triangles(LGObject* pObj, const ug::vector4& color,
						  ug::FaceIterator trisBegin,
						  ug::FaceIterator trisEnd,
						  ug::Grid::VertexAttachmentAccessor<APosition>& aaPos,
						  ug::Grid::FaceAttachmentAccessor<ANormal>& aaNorm)
{
	Grid::FaceAttachmentAccessor<ABool> aaRenderedFACE(pObj->grid(), m_aRendered);

	glColor4f(color.x(), color.y(), color.z(), color.w());
	glBegin(GL_TRIANGLES);

	for(FaceIterator iter = trisBegin;
		iter != trisEnd; ++iter)
	{
		Face* tri = *iter;

		if(aaRenderedFACE[tri]){
			vector3& n = aaNorm[tri];
			glNormal3f(n.x(), n.y(), n.z());

			for(int i = 0; i < 3; ++i)
			{
				vector3& v = aaPos[tri->vertex(i)];
				glVertex3f(v.x(), v.y(), v.z());
			}
		}
	}

	glEnd();
}

void LGScene::render_quadrilaterals(LGObject* pObj,
						  const ug::vector4& color,
						  ug::FaceIterator quadsBegin,
						  ug::FaceIterator quadsEnd,
						  ug::Grid::VertexAttachmentAccessor<APosition>& aaPos,
						  ug::Grid::FaceAttachmentAccessor<ANormal>& aaNorm)
{
	Grid::FaceAttachmentAccessor<ABool> aaRenderedFACE(pObj->grid(), m_aRendered);

	glColor4f(color.x(), color.y(), color.z(), color.w());
	glBegin(GL_QUADS);

	for(FaceIterator iter = quadsBegin;
		iter != quadsEnd; ++iter)
	{
		Face* q = *iter;

		if(aaRenderedFACE[q]){
			vector3& n = aaNorm[q];
			glNormal3f(n.x(), n.y(), n.z());

			for(int i = 0; i < 4; ++i)
			{
				vector3& v = aaPos[q->vertex(i)];
				glVertex3f(v.x(), v.y(), v.z());
			}
		}
	}

	glEnd();
}

void LGScene::rerender_volumes(LGObject* pObj,
						  const ug::vector4& color,
						  ug::VolumeIterator volsBegin,
						  ug::VolumeIterator volsEnd,
						  ug::Grid::VertexAttachmentAccessor<APosition>& aaPos,
						  ug::Grid::FaceAttachmentAccessor<ANormal>& aaNorm)
{
	Grid& grid = pObj->grid();
	Grid::FaceAttachmentAccessor<ABool> aaRenderedFACE(grid, m_aRendered);
	Grid::VolumeAttachmentAccessor<ABool> aaRenderedVOL(grid, m_aRendered);

//	collect all triangles and quadrilaterals that have to be rendered
	vector<Face*> tris;
	vector<Face*> quads;
	Grid::face_traits::secure_container	assFaces;
	for(VolumeIterator iter = volsBegin; iter != volsEnd; ++iter)
	{
		Volume* vol = *iter;
		if(aaRenderedVOL[vol]){
		//	iterate through the volumes faces
			grid.associated_elements(assFaces, vol);
			// Grid::AssociatedFaceIterator fEnd = grid.associated_faces_end(vol);
			// for(Grid::AssociatedFaceIterator fIter = grid.associated_faces_begin(vol);
			// 	fIter != fEnd; ++fIter)
			for(size_t iface = 0; iface < assFaces.size(); ++iface){
				// Face* f = *fIter;
				Face* f = assFaces[iface];
				if(aaRenderedFACE[f]){
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

	if(tris.size() > 0){
		glBegin(GL_TRIANGLES);

		for(size_t j = 0; j < tris.size(); ++j){
			Face* tri = tris[j];

			vector3& n = aaNorm[tri];
			glNormal3f(n.x(), n.y(), n.z());

			for(int i = 0; i < 3; ++i){
				vector3& v = aaPos[tri->vertex(i)];
				glVertex3f(v.x(), v.y(), v.z());
			}
		}

		glEnd();
	}

	if(quads.size() > 0){
		glBegin(GL_QUADS);

		for(size_t j = 0; j < quads.size(); ++j){
			Face* q = quads[j];

			vector3& n = aaNorm[q];
			glNormal3f(n.x(), n.y(), n.z());

			for(int i = 0; i < 4; ++i){
				vector3& v = aaPos[q->vertex(i)];
				glVertex3f(v.x(), v.y(), v.z());
			}
		}

		glEnd();
	}
}

void LGScene::render_faces(LGObject* pObj, Grid& grid,
						   SubsetHandler& sh, bool renderAll)
{
	PROFILE_FUNC();
//	this method allows to fill the contents of the display lists
//	with differing grids and subsets.
	Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);
	Grid::FaceAttachmentAccessor<ANormal> aaNorm(grid, aNormal);
	Grid::VertexAttachmentAccessor<ABool> aaRenderedVRT(grid, m_aRendered);
	Grid::EdgeAttachmentAccessor<ABool> aaRenderedEDGE(grid, m_aRendered);
	Grid::FaceAttachmentAccessor<ABool> aaRenderedFACE(grid, m_aRendered);

	Grid::FaceAttachmentAccessor<ABool> aaHidden(grid, m_aHidden);

//	iterate through all subsets
//	each subset has its own display list
	Grid::edge_traits::secure_container	assEdges;

	for(int i = 0; i < sh.num_subsets(); ++i)
	{
	//	set up open-gl display lists
		GLuint displayList = pObj->get_display_list(i);
		pObj->set_display_list_mode(i, LGRM_DOUBLE_PASS_SHADED);
		glDeleteLists(displayList, 1);
		glNewList(displayList, GL_COMPILE);

		if((!renderAll) && (!pObj->subset_is_visible(i))){
			glEndList();
			continue;
		}

	//	draw triangles
		glBegin(GL_TRIANGLES);

		for(FaceIterator iter = sh.begin<Triangle>(i);
			iter != sh.end<Triangle>(i); ++iter)
		{
			Face* tri = *iter;
			if(aaHidden[tri] && !renderAll)
				continue;

			aaRenderedFACE[tri] = true;

			vector3& n = aaNorm[tri];
			glNormal3f(n.x(), n.y(), n.z());

			for(int i = 0; i < 3; ++i)
			{
				aaRenderedVRT[tri->vertex(i)] = true;
				vector3& v = aaPos[tri->vertex(i)];
				glVertex3f(v.x(), v.y(), v.z());
			}

			// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(tri);
			// 	eiter != grid.associated_edges_end(tri); ++eiter)
			grid.associated_elements(assEdges, tri);
			for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
				aaRenderedEDGE[assEdges[iedge]] = true;
		}

		for(FaceIterator iter = sh.begin<ConstrainingTriangle>(i);
			iter != sh.end<ConstrainingTriangle>(i); ++iter)
		{
			Face* tri = *iter;
			if(aaHidden[tri] && !renderAll)
				continue;

			aaRenderedFACE[tri] = true;

			vector3& n = aaNorm[tri];
			glNormal3f(n.x(), n.y(), n.z());

			for(int i = 0; i < 3; ++i)
			{
				aaRenderedVRT[tri->vertex(i)] = true;
				vector3& v = aaPos[tri->vertex(i)];
				glVertex3f(v.x(), v.y(), v.z());
			}

			// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(tri);
			// 	eiter != grid.associated_edges_end(tri); ++eiter)
			// 	aaRenderedEDGE[*eiter] = true;
			grid.associated_elements(assEdges, tri);
			for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
				aaRenderedEDGE[assEdges[iedge]] = true;
		}

		for(FaceIterator iter = sh.begin<ConstrainedTriangle>(i);
			iter != sh.end<ConstrainedTriangle>(i); ++iter)
		{
			Face* tri = *iter;
			if(aaHidden[tri] && !renderAll)
				continue;

			aaRenderedFACE[tri] = true;

			vector3& n = aaNorm[tri];
			glNormal3f(n.x(), n.y(), n.z());

			for(int i = 0; i < 3; ++i)
			{
				aaRenderedVRT[tri->vertex(i)] = true;
				vector3& v = aaPos[tri->vertex(i)];
				glVertex3f(v.x(), v.y(), v.z());
			}

			// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(tri);
			// 	eiter != grid.associated_edges_end(tri); ++eiter)
			// 	aaRenderedEDGE[*eiter] = true;
			grid.associated_elements(assEdges, tri);
			for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
				aaRenderedEDGE[assEdges[iedge]] = true;
		}

		glEnd();

	//	draw quads
		glBegin(GL_QUADS);

		for(QuadrilateralIterator iter = sh.begin<Quadrilateral>(i);
			iter != sh.end<Quadrilateral>(i); ++iter)
		{
			Quadrilateral* q = *iter;
			if(aaHidden[q] && !renderAll)
				continue;

			aaRenderedFACE[q] = true;

			vector3& n = aaNorm[q];
			glNormal3f(n.x(), n.y(), n.z());

			for(int i = 0; i < 4; ++i)
			{
				aaRenderedVRT[q->vertex(i)] = true;
				vector3& v = aaPos[q->vertex(i)];
				glVertex3f(v.x(), v.y(), v.z());
			}

			// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(q);
			// 	eiter != grid.associated_edges_end(q); ++eiter)
			// 	aaRenderedEDGE[*eiter] = true;
			grid.associated_elements(assEdges, q);
			for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
				aaRenderedEDGE[assEdges[iedge]] = true;
		}

		for(ConstrainingQuadrilateralIterator iter = sh.begin<ConstrainingQuadrilateral>(i);
			iter != sh.end<ConstrainingQuadrilateral>(i); ++iter)
		{
			ConstrainingQuadrilateral* q = *iter;
			if(aaHidden[q] && !renderAll)
				continue;

			aaRenderedFACE[q] = true;

			vector3& n = aaNorm[q];
			glNormal3f(n.x(), n.y(), n.z());

			for(int i = 0; i < 4; ++i)
			{
				aaRenderedVRT[q->vertex(i)] = true;
				vector3& v = aaPos[q->vertex(i)];
				glVertex3f(v.x(), v.y(), v.z());
			}

			// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(q);
			// 	eiter != grid.associated_edges_end(q); ++eiter)
			// 	aaRenderedEDGE[*eiter] = true;
			grid.associated_elements(assEdges, q);
			for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
				aaRenderedEDGE[assEdges[iedge]] = true;
		}

		for(ConstrainedQuadrilateralIterator iter = sh.begin<ConstrainedQuadrilateral>(i);
			iter != sh.end<ConstrainedQuadrilateral>(i); ++iter)
		{
			ConstrainedQuadrilateral* q = *iter;
			if(aaHidden[q] && !renderAll)
				continue;

			aaRenderedFACE[q] = true;

			vector3& n = aaNorm[q];
			glNormal3f(n.x(), n.y(), n.z());

			for(int i = 0; i < 4; ++i)
			{
				aaRenderedVRT[q->vertex(i)] = true;
				vector3& v = aaPos[q->vertex(i)];
				glVertex3f(v.x(), v.y(), v.z());
			}

			// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(q);
			// 	eiter != grid.associated_edges_end(q); ++eiter)
			// 	aaRenderedEDGE[*eiter] = true;
			grid.associated_elements(assEdges, q);
			for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
				aaRenderedEDGE[assEdges[iedge]] = true;
		}

		glEnd();
		glEndList();
	}
}

void LGScene::render_faces_without_clip_plane(LGObject* pObj)
{
//	renders the faces of an object.
//	visibility is handled by the draw routine.
	render_faces(pObj, pObj->grid(), pObj->subset_handler());
}

void LGScene::render_volumes(LGObject* pObj)
{
	PROFILE_FUNC();
//	renders the volumes of an object.
//	clip planes are used.
	Grid& grid = pObj->grid();
	SubsetHandler& sh = pObj->subset_handler();

	Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);
	Grid::FaceAttachmentAccessor<ANormal> aaNorm(grid, aNormal);
	Grid::FaceAttachmentAccessor<ASphere>	aaSphereFACE(grid, m_aSphere);
	Grid::VolumeAttachmentAccessor<ASphere>	aaSphereVOL(grid, m_aSphere);

	Grid::FaceAttachmentAccessor<ABool> aaRenderedFACE(grid, m_aRendered);
	Grid::VolumeAttachmentAccessor<ABool> aaRenderedVOL(grid, m_aRendered);
	Grid::VolumeAttachmentAccessor<ABool> aaHiddenVOL(grid, m_aHidden);

//	preprocessing step:
//	sort all faces in a subset handler
	SubsetHandler& shFace = pObj->m_shFacesForVolRendering;
	shFace.clear();

	Grid::volume_traits::secure_container assVols;

//	too slow too!
//	iterate through all faces
	for(FaceIterator iter = grid.faces_begin();
		iter != grid.faces_end(); ++iter)
	{
		Face* f = *iter;
	//	check whether the face is clipped.
		if(!clip_face(f, aaSphereFACE[f], aaPos))
		{
		//	it's not.
		//	if it has exactly one visible adjacent volume, or no adjacent volumes at all,
		//	then it has to be displayed
			Volume* visVol = NULL;
			int newSubInd = -1;

			int fSubInd = sh.get_subset_index(f);
			bool faceIsVisible = (fSubInd != -1) && m_drawFaces && pObj->subset_is_visible(fSubInd);
			
			// Grid::AssociatedVolumeIterator volEnd = grid.associated_volumes_end(f);
			// Grid::AssociatedVolumeIterator volBegin = grid.associated_volumes_begin(f);
			grid.associated_elements(assVols, f);
			if(assVols.empty()){
			//	the face has to be rendered, since it is not adjacent to any volume
				if(faceIsVisible)
					shFace.assign_subset(f, fSubInd);
			}
			else{
				int numVisVols = 0;
				
				// for(Grid::AssociatedVolumeIterator vIter = volBegin;
				// 	vIter != volEnd; ++vIter)
				for(size_t ivol = 0; ivol < assVols.size(); ++ivol){
					Volume* assVol = assVols[ivol];
					if(aaHiddenVOL[assVol])
						continue;

					int vSubInd = sh.get_subset_index(assVol);
				
					if(vSubInd == -1)
						continue;

				//	check whether the subset of the volume is visible
					if(pObj->subset_is_visible(vSubInd))
					{
					//	make sure the volume is not clipped.
					//	uncomment the following line for exact clipping tests.
						if(!clip_volume(assVol, aaSphereVOL[assVol], aaPos))
						{
							++numVisVols;
						//	if newSubInd has already been assigned, we'll reset it to 0
							if(newSubInd != -1){
								newSubInd = -1;
								visVol = NULL;
							}
							else{
								newSubInd = vSubInd;
								visVol = assVol;
							}
						}
					}
				}

				//	if the face and volume subsets do not match, and if the face
				//	is visible, we'll simply draw the face itself.
				if((numVisVols < 2) && faceIsVisible){
					shFace.assign_subset(f, fSubInd);
					if(visVol)
						aaRenderedVOL[visVol] = true;
				}
				else if(newSubInd != -1){
				//	if newSubInd != -1 then the face has to be drawn.
					shFace.assign_subset(f, newSubInd);
				//	mark the visible volume as rendered
					if(visVol){
						aaRenderedVOL[visVol] = true;
					}
				}
			}
		}
	}

/*too slow!
//	iterate through all subsets
	for(int i = 0; i < sh.num_subsets(); ++i)
	{
	//	check whether the subset is visible
		if(pObj->subset_is_visible(i))
		{
		//	iterate through the volumes
			for(VolumeIterator iter = sh.begin<Volume>(i);
				iter != sh.end<Volume>(i); ++iter)
			{
				Volume* vol = *iter;
				bool visible = true;
			//	check whether the volume is clipped by a clipping-plane
				RelativePositionIndicator rpi = clip_sphere(aaSphereVOL[vol]);
				if(rpi == RPI_CUT || rpi == RPI_OUTSIDE_TOUCHES || rpi == RPI_OUTSIDE)
				{
				//	check if there is at least one vertex on the outer side.
					uint numVrts = vol->num_vertices();
					for(uint j = 0; j < numVrts; ++j)
					{
						if(clip_point(aaPos[vol->vertex(j)]) == RPI_OUTSIDE)
						{
							visible = false;
							break;
						}
					}
				}

			//	if the volume is visible then the associated faces are potential
			//	candidates for drawing.
				if(visible)
				{
					FaceIterator iterEnd = grid.associated_faces_end(vol);
					for(FaceIterator fIter = grid.associated_faces_begin(vol);
						fIter != iterEnd; ++fIter)
					{
					//	if the face is already assigned to a subset of shFace,
					//	then we'll unassign it again. If not, we'll assign it
					//	to the subset of the volume we're regarding.
						if(shFace.get_subset_index(*fIter) == -1)
							shFace.assign_subset(*fIter, i);
						else
							shFace.assign_subset(*fIter, -1);
					}
				}
			}
		}
	}
*/

//	make sure that shFace contains at as many subsets as the grids subset-handler
	if(shFace.num_subsets() < sh.num_subsets())
		shFace.set_subset_info(sh.num_subsets() - 1, SubsetInfo());

//	finally render the faces that we collected in the subset handler.
	render_faces(pObj, grid, shFace, true);
}

void LGScene::render_faces_with_clip_plane(LGObject* pObj)
{
//	renders the faces of an object.
//	visibility is handled by the draw routine.
	Grid& grid = pObj->grid();
	SubsetHandler& sh = pObj->subset_handler();

	Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);
	Grid::FaceAttachmentAccessor<ANormal> aaNorm(grid, aNormal);
	Grid::FaceAttachmentAccessor<ASphere>	aaSphereFACE(grid, m_aSphere);
	Grid::VolumeAttachmentAccessor<ASphere>	aaSphereVOL(grid, m_aSphere);

	Grid::VertexAttachmentAccessor<ABool> aaRenderedVRT(grid, m_aRendered);
	Grid::EdgeAttachmentAccessor<ABool> aaRenderedEDGE(grid, m_aRendered);
	Grid::FaceAttachmentAccessor<ABool> aaRenderedFACE(grid, m_aRendered);

	Grid::FaceAttachmentAccessor<ABool> aaHidden(grid, m_aHidden);

	SetAttachmentValues(aaRenderedVRT, grid.vertices_begin(),
						grid.vertices_end(), false);
	SetAttachmentValues(aaRenderedEDGE, grid.edges_begin(),
						grid.edges_end(), false);
	SetAttachmentValues(aaRenderedFACE, grid.faces_begin(),
						grid.faces_end(), false);

//	iterate through all subsets
//	each subset has its own display list
	Grid::edge_traits::secure_container	assEdges;

	for(int i = 0; i < sh.num_subsets(); ++i)
	{
	//	set up open-gl display lists
		GLuint displayList = pObj->get_display_list(i);
		pObj->set_display_list_mode(i, LGRM_DOUBLE_PASS_SHADED);
		glDeleteLists(displayList, 1);
		glNewList(displayList, GL_COMPILE);

		if(!pObj->subset_is_visible(i)){
			glEndList();
			continue;
		}

	//	draw triangles
		if(sh.num<Triangle>(i) > 0 ||
			sh.num<ConstrainingTriangle>(i) > 0 ||
			sh.num<ConstrainedTriangle>(i) > 0)
		{
			glBegin(GL_TRIANGLES);

			for(TriangleIterator iter = sh.begin<Triangle>(i);
				iter != sh.end<Triangle>(i); ++iter)
			{
				Triangle* tri = *iter;
				if(aaHidden[tri])
					continue;

				if(!clip_face(tri, aaSphereFACE[tri], aaPos))
				{
					aaRenderedFACE[tri] = true;
					vector3& n = aaNorm[tri];
					glNormal3f(n.x(), n.y(), n.z());

					for(int j = 0; j < 3; ++j)
					{
						aaRenderedVRT[tri->vertex(j)] = true;
						vector3& v = aaPos[tri->vertex(j)];
						glVertex3f(v.x(), v.y(), v.z());
					}
					// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(tri);
					// 	eiter != grid.associated_edges_end(tri); ++eiter)
					// 		aaRenderedEDGE[*eiter] = true;
					grid.associated_elements(assEdges, tri);
					for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
						aaRenderedEDGE[assEdges[iedge]] = true;
				}
			}
			for(FaceIterator iter = sh.begin<ConstrainingTriangle>(i);
				iter != sh.end<ConstrainingTriangle>(i); ++iter)
			{
				Face* tri = *iter;
				if(aaHidden[tri])
					continue;

				if(!clip_face(tri, aaSphereFACE[tri], aaPos))
				{
					aaRenderedFACE[tri] = true;

					vector3& n = aaNorm[tri];
					glNormal3f(n.x(), n.y(), n.z());

					for(int i = 0; i < 3; ++i)
					{
						aaRenderedVRT[tri->vertex(i)] = true;
						vector3& v = aaPos[tri->vertex(i)];
						glVertex3f(v.x(), v.y(), v.z());
					}

					// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(tri);
					// 	eiter != grid.associated_edges_end(tri); ++eiter)
					// 		aaRenderedEDGE[*eiter] = true;
					grid.associated_elements(assEdges, tri);
					for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
						aaRenderedEDGE[assEdges[iedge]] = true;
				}
			}

			for(FaceIterator iter = sh.begin<ConstrainedTriangle>(i);
				iter != sh.end<ConstrainedTriangle>(i); ++iter)
			{
				Face* tri = *iter;
				if(aaHidden[tri])
					continue;

				if(!clip_face(tri, aaSphereFACE[tri], aaPos))
				{
					aaRenderedFACE[tri] = true;

					vector3& n = aaNorm[tri];
					glNormal3f(n.x(), n.y(), n.z());

					for(int i = 0; i < 3; ++i)
					{
						aaRenderedVRT[tri->vertex(i)] = true;
						vector3& v = aaPos[tri->vertex(i)];
						glVertex3f(v.x(), v.y(), v.z());
					}

					// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(tri);
					// 	eiter != grid.associated_edges_end(tri); ++eiter)
					// 		aaRenderedEDGE[*eiter] = true;
					grid.associated_elements(assEdges, tri);
					for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
						aaRenderedEDGE[assEdges[iedge]] = true;
				}
			}

			glEnd();
		}

	//	draw quads
		if(sh.num<Quadrilateral>(i) > 0)
		{
			glBegin(GL_QUADS);

			for(FaceIterator iter = sh.begin<Quadrilateral>(i);
				iter != sh.end<Quadrilateral>(i); ++iter)
			{
				Face* q = *iter;
				if(aaHidden[q])
					continue;

				if(!clip_face(q, aaSphereFACE[q], aaPos))
				{
					aaRenderedFACE[q] = true;
					vector3& n = aaNorm[q];
					glNormal3f(n.x(), n.y(), n.z());

					for(int j = 0; j < 4; ++j)
					{
						aaRenderedVRT[q->vertex(j)] = true;
						vector3& v = aaPos[q->vertex(j)];
						glVertex3f(v.x(), v.y(), v.z());
					}
					// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(q);
					// 	eiter != grid.associated_edges_end(q); ++eiter)
					// 		aaRenderedEDGE[*eiter] = true;
					grid.associated_elements(assEdges, q);
					for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
						aaRenderedEDGE[assEdges[iedge]] = true;
				}
			}

			for(FaceIterator iter = sh.begin<ConstrainingQuadrilateral>(i);
				iter != sh.end<ConstrainingQuadrilateral>(i); ++iter)
			{
				Face* q = *iter;
				if(aaHidden[q])
					continue;

				if(!clip_face(q, aaSphereFACE[q], aaPos))
				{
					aaRenderedFACE[q] = true;
					vector3& n = aaNorm[q];
					glNormal3f(n.x(), n.y(), n.z());

					for(int j = 0; j < 4; ++j)
					{
						aaRenderedVRT[q->vertex(j)] = true;
						vector3& v = aaPos[q->vertex(j)];
						glVertex3f(v.x(), v.y(), v.z());
					}

					// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(q);
					// 	eiter != grid.associated_edges_end(q); ++eiter)
					// 		aaRenderedEDGE[*eiter] = true;
					grid.associated_elements(assEdges, q);
					for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
						aaRenderedEDGE[assEdges[iedge]] = true;
				}
			}

			for(FaceIterator iter = sh.begin<ConstrainedQuadrilateral>(i);
				iter != sh.end<ConstrainedQuadrilateral>(i); ++iter)
			{
				Face* q = *iter;
				if(aaHidden[q])
					continue;

				if(!clip_face(q, aaSphereFACE[q], aaPos))
				{
					aaRenderedFACE[q] = true;
					vector3& n = aaNorm[q];
					glNormal3f(n.x(), n.y(), n.z());

					for(int j = 0; j < 4; ++j)
					{
						aaRenderedVRT[q->vertex(j)] = true;
						vector3& v = aaPos[q->vertex(j)];
						glVertex3f(v.x(), v.y(), v.z());
					}

					// for(Grid::AssociatedEdgeIterator eiter = grid.associated_edges_begin(q);
					// 	eiter != grid.associated_edges_end(q); ++eiter)
					// 		aaRenderedEDGE[*eiter] = true;
					grid.associated_elements(assEdges, q);
					for(size_t iedge = 0; iedge < assEdges.size(); ++iedge)
						aaRenderedEDGE[assEdges[iedge]] = true;
				}
			}
			glEnd();
		}
/*
		if(sh.num<Volume>(i) > 0)
		{
		//	iterate through volumes
			for(VolumeIterator iter = sh.begin<Volume>(i);
				iter != sh.end<Volume>(i); ++iter)
			{
				Volume* vol = *iter;
				bool bDraw = false;

			//	associated faces are only drawn, if the volume cuts the clip-plane.
				if(clip_sphere(aaSphereVOL[vol]) == RPI_CUT)
				{
				//	check if there are more than two vertices on the inner
				//	and at least one vertex on the outer side.
					uint numVrts = vol->num_vertices();
					bool bGotOuter = false;
					int numInner = 0;
					for(uint j = 0; j < numVrts; ++j)
					{
						RelativePositionIndicator rpi = clip_point(aaPos[vol->vertex(j)]);
						if(rpi == RPI_OUTSIDE)
							bGotOuter = true;
						else
							++numInner;
					}

					bDraw = bGotOuter && (numInner > 2);
				}

				if(bDraw)
				{
				//	iterate through associated faces
					Grid::AssociatedFaceIterator iterEnd = grid.associated_faces_end(vol);
					for(Grid::AssociatedFaceIterator fIter = grid.associated_faces_begin(vol);
						fIter != iterEnd; ++fIter)
					{
						Face* f = *fIter;
					//	the face will only be drawn if its subset index is -1
						if(sh.get_subset_index(f) == -1)
						{
						//	check whether the face has to be clipped
							if(!clip_face(f, aaSphereFACE[f], aaPos))
							{
								uint numVrts = f->num_vertices();

							//	gl-begin statement
								if(numVrts == 3)
									glBegin(GL_TRIANGLES);
								else if(numVrts == 4)
									glBegin(GL_QUADS);
								else
									continue;//	only faces with 3 or 4 vertices are supported

							//	draw the face
								vector3& n = aaNorm[f];
								glNormal3f(n.x(), n.y(), n.z());

								for(uint j = 0; j < numVrts; ++j)
								{
									vector3& v = aaPos[f->vertex(j)];
									glVertex3f(v.x(), v.y(), v.z());
								}

								glEnd();
							}
						}
					}
				}
			}
		}*/
		glEndList();
	}
}

/*
Don't forget subset indices and draw order!

1:	tet:(numFaces + 4*numVolumes)
	hex:(numFaces + 6*numVolumes)

	iterate through all faces
		set tmp_ind to -1
		iterate through associated volumes
			if tmp_ind == -1
				tmp_ind = subset_index_of_volume
			else
				tmp_ind = -1
*/
/*
2:
	attach subset-handler shFace

	iterate through all volumes
		if volume is visible
			iterate through associated faces
				if shFace(face) == -1
					shFace(face) = subset_index_of_volume
				else
					shFace(face) = -1
*/

ug::Vertex* LGScene::
get_clicked_vertex(LGObject* obj, const ug::vector3& from,
				   const ug::vector3& to)
{
	Vertex* vrtClosest = NULL;

	if(obj){
		Grid& grid = obj->grid();
		Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);
		Grid::VertexAttachmentAccessor<ABool> aaRenderedVRT(grid, m_aRendered);

	//	max distance - a safe overestimation
		number minDist = m_zFar * 2.;

		for(VertexIterator iter = grid.begin<Vertex>();
			iter != grid.end<Vertex>(); ++iter)
		{
			Vertex* vrt = *iter;
			if(aaRenderedVRT[vrt]){
				number t;
				number dist = DistancePointToLine(t, aaPos[vrt], from, to);
				if(dist < minDist && t > 0 && t < 1.2){
					vrtClosest = vrt;
					minDist = dist;
				}
			}
		}
	}

	return vrtClosest;
}

ug::Edge* LGScene::
get_clicked_edge(LGObject* obj, const ug::vector3& from,
				 const ug::vector3& to, bool closestToTo)
{
	Edge* eClosest = NULL;

	if(obj){
	//	iterate through the edges and check the center of each against
	//	the ray.
		Grid& grid = obj->grid();
		Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);
		Grid::EdgeAttachmentAccessor<ABool> aaRenderedEDGE(grid, m_aRendered);

	//	max distance - a safe overestimation
		number minDist = m_zFar * 2.;
		vector3 dir;
		VecSubtract(dir, to, from);

		if(closestToTo){
		//	calculate the minimal distance of each edge to 'to'
			for(EdgeIterator iter = grid.begin<Edge>();
				iter != grid.end<Edge>(); ++iter)
			{
				Edge* e = *iter;
				if(!aaRenderedEDGE[e])
					continue;
				number t;
				number dist = DistancePointToLine(t, to, aaPos[e->vertex(0)],
													aaPos[e->vertex(1)]);
				if(dist < minDist && t > -0.2 && t < 1.2){
					eClosest = e;
					minDist = dist;
				}
			}
		}
		else{
			number minDistSq = minDist * minDist;

		//	calculate the minimal distance of the center of each edge
		//	to the ray (from, to)
			for(EdgeIterator iter = grid.begin<Edge>();
				iter != grid.end<Edge>(); ++iter)
			{
				Edge* e = *iter;
				if(!aaRenderedEDGE[e])
					continue;

			//todo:	make sure that at least one of the endpoints lies in front of the
			//	near-plane.
				vector3 isectA, isectB;
				LineLineIntersection3d(isectA, isectB,
									   aaPos[e->vertex(0)], aaPos[e->vertex(1)],
									   from, to);

			//	check whether isectA is in front of the near plane
				vector3 isectDir;
				VecSubtract(isectDir, isectA, from);

				if((VecDot(isectDir, dir) > 0)){
				//	distance between the two lines
					number distSq = VecDistanceSq(isectA, isectB);
					if(distSq < minDistSq){
						eClosest = e;
						minDistSq = distSq;
					}
				}
			}
		}
	}

	return eClosest;
}

ug::Face* LGScene::
get_clicked_face(LGObject* pObj, const ug::vector3& from,
				 const ug::vector3& to)
{
	vector3 dir;
	VecSubtract(dir, to, from);

	Grid& grid = pObj->grid();
//	SubsetHandler& sh = pObj->subset_handler();

	Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);
	Grid::FaceAttachmentAccessor<ANormal> aaNorm(grid, aNormal);
	Grid::FaceAttachmentAccessor<ASphere>	aaSphereFACE(grid, m_aSphere);

	Grid::FaceAttachmentAccessor<ABool> aaRenderedFACE(grid, m_aRendered);

	Face* clickedFace = NULL;
	number maxDistSq = m_zFar * 2.;
	maxDistSq *= maxDistSq;

//	iterate through all faces
//	if the given ray cuts the triangles sphere, we'll examine it closer.
	for(FaceIterator iter = grid.faces_begin();
		iter != grid.faces_end(); ++iter)
	{
		Face* f =*iter;

	//	make sure that the face is visible
		if(!aaRenderedFACE[f])
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
		number normDot = VecDot(aaNorm[f], dir);
		if(!(m_drawModeBack & DM_SOLID)){
			if(normDot > 0)
				continue;
		}
		if(!(m_drawModeFront & DM_SOLID)){
			if(normDot < 0)
				continue;
		}

	//	check bounding sphere
		Sphere3& sphere = aaSphereFACE[f];
		number t;
		if(DistancePointToLine(t, sphere.get_center(), from, to)
			<= sphere.get_radius())
		{
			bool intersecting = false;
		//	perform line-face check
			vector3 v;
			number bc1, bc2, t;

			if(f->num_vertices() == 3){
				intersecting = RayTriangleIntersection(v, bc1, bc2, t,
													aaPos[f->vertex(0)],
													aaPos[f->vertex(1)],
													aaPos[f->vertex(2)],
													from, dir);
			}
			else if(f->num_vertices() == 4)
			{
				intersecting = RayTriangleIntersection(v, bc1, bc2, t,
													aaPos[f->vertex(0)],
													aaPos[f->vertex(1)],
													aaPos[f->vertex(2)],
													from, dir);
				if(!intersecting){
					intersecting = RayTriangleIntersection(v, bc1, bc2, t,
													aaPos[f->vertex(0)],
													aaPos[f->vertex(2)],
													aaPos[f->vertex(3)],
													from, dir);
				}
			}

			if(intersecting)
			{
				if(t > 0){
					number distSq = VecDistanceSq(v, from);
					if(distSq < maxDistSq){
					//	we found a face that's closer than the current one.
						clickedFace = f;
						maxDistSq = distSq;
					}
				}
			}
		}
	}

	return clickedFace;
}

ug::Volume* LGScene::
get_clicked_volume(LGObject* pObj, const ug::vector3& from,
					const ug::vector3& to)
{
//	get the clicked face and check its associated volumes.
//	if a visible volume is associated, it is considered to be clicked.
	Grid& grid = pObj->grid();
	SubsetHandler& sh = pObj->subset_handler();
	Grid::VolumeAttachmentAccessor<ABool> aaRenderedVOL(grid, m_aRendered);

//	get the clicked face.
	Face* clickedFace = get_clicked_face(pObj, from, to);

//	if a face was clicked we'll check associated volumes
	if(clickedFace){
		vector<Volume*> vVols;
		CollectVolumes(vVols, grid, clickedFace);

		for(size_t i = 0; i < vVols.size(); ++i){
			Volume* vol = vVols[i];
			if(aaRenderedVOL[vol]){
				int si = sh.get_subset_index(vol);
				if(pObj->subset_is_visible(si))
					return vol;
			}
		}
	}

//	nothing seems to be clicked
	return NULL;
}

size_t LGScene::
get_vertices_in_rect(std::vector<Vertex*>& vrtsOut,
					LGObject* obj,
					float xMin, float yMin, float xMax, float yMax)
{
//	iterate over all vertices and select them if they are visible
	vrtsOut.clear();
	yMin = m_viewHeight - yMin;
	yMax = m_viewHeight - yMax;
	swap(yMin, yMax);

	if(obj)
	{
		GLdouble modelMat[16];
		GLdouble projMat[16];
		GLint viewport[4];

		glGetDoublev(GL_MODELVIEW_MATRIX, modelMat);
		glGetDoublev(GL_PROJECTION_MATRIX, projMat);
		glGetIntegerv(GL_VIEWPORT, viewport);

		GLdouble vx, vy, vz;
		
		Grid& grid = obj->grid();
		Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);
		Grid::VertexAttachmentAccessor<ABool> aaRenderedVRT(grid, m_aRendered);

		Plane plane = near_clip_plane();
		for(VertexIterator iter = grid.begin<Vertex>();
			iter != grid.end<Vertex>(); ++iter)
		{
			Vertex* vrt = *iter;
			if(aaRenderedVRT[vrt]){
				vector3& pos = aaPos[vrt];
				
				if(PlanePointTest(plane, pos) == RPI_INSIDE)
					continue;
					
				GLdouble px = pos.x();
				GLdouble py = pos.y();
				GLdouble pz = pos.z();

				gluProject(px, py, pz, modelMat, projMat,
		   				   viewport, &vx, &vy, &vz);
				
				if(vx >= xMin && vx <= xMax && vy >= yMin && vy <= yMax && vz >= 0){
					vrtsOut.push_back(vrt);
				}
			}
		}
	}

	return vrtsOut.size();
}

size_t LGScene::
get_edges_in_rect(std::vector<Edge*>& edgesOut,
					LGObject* obj,
					float xMin, float yMin, float xMax, float yMax)
{
	edgesOut.clear();
	yMin = m_viewHeight - yMin;
	yMax = m_viewHeight - yMax;
	swap(yMin, yMax);

	if(obj)
	{
		GLdouble modelMat[16];
		GLdouble projMat[16];
		GLint viewport[4];

		glGetDoublev(GL_MODELVIEW_MATRIX, modelMat);
		glGetDoublev(GL_PROJECTION_MATRIX, projMat);
		glGetIntegerv(GL_VIEWPORT, viewport);

		GLdouble vx, vy, vz;
		Grid& grid = obj->grid();
		Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);
		Grid::EdgeAttachmentAccessor<ABool> aaRenderedEDGE(grid, m_aRendered);
		Plane plane = near_clip_plane();
		
		for(EdgeIterator iter = grid.begin<Edge>();
			iter != grid.end<Edge>(); ++iter)
		{
			Edge* e = *iter;
			if(!aaRenderedEDGE[e])
				continue;

			bool allIn = true;
			for(size_t i = 0; i < e->num_vertices(); ++i){
				Vertex* vrt = e->vertex(i);
				vector3& pos = aaPos[vrt];
				
				if(PlanePointTest(plane, pos) == RPI_INSIDE){
					allIn = false;
					break;
				}
					
				GLdouble px = pos.x();
				GLdouble py = pos.y();
				GLdouble pz = pos.z();

				gluProject(px, py, pz, modelMat, projMat,
						   viewport, &vx, &vy, &vz);

				if(vx < xMin || vx > xMax || vy < yMin || vy > yMax || vz < 0){
					allIn = false;
					break;
				}
			}
			if(allIn)
				edgesOut.push_back(e);
		}
	}

	return edgesOut.size();
}

size_t LGScene::
get_faces_in_rect(std::vector<Face*>& facesOut,
					LGObject* obj,
					float xMin, float yMin, float xMax, float yMax)
{
	facesOut.clear();
	yMin = m_viewHeight - yMin;
	yMax = m_viewHeight - yMax;
	swap(yMin, yMax);

	if(obj)
	{
		GLdouble modelMat[16];
		GLdouble projMat[16];
		GLint viewport[4];

		glGetDoublev(GL_MODELVIEW_MATRIX, modelMat);
		glGetDoublev(GL_PROJECTION_MATRIX, projMat);
		glGetIntegerv(GL_VIEWPORT, viewport);

		GLdouble vx, vy, vz;
		Grid& grid = obj->grid();
		Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);
		Grid::FaceAttachmentAccessor<ABool> aaRenderedFACE(grid, m_aRendered);
		Plane plane = near_clip_plane();
		
		for(FaceIterator iter = grid.begin<Face>();
			iter != grid.end<Face>(); ++iter)
		{
			Face* f = *iter;
			if(!aaRenderedFACE[f])
				continue;

			bool allIn = true;
			for(size_t i = 0; i < f->num_vertices(); ++i){
				Vertex* vrt = f->vertex(i);
				vector3& pos = aaPos[vrt];
				if(PlanePointTest(plane, pos) == RPI_INSIDE){
					allIn = false;
					break;
				}
				
				GLdouble px = pos.x();
				GLdouble py = pos.y();
				GLdouble pz = pos.z();

				gluProject(px, py, pz, modelMat, projMat,
						   viewport, &vx, &vy, &vz);

				if(vx < xMin || vx > xMax || vy < yMin || vy > yMax || vz < 0){
					allIn = false;
					break;
				}
			}
			if(allIn)
				facesOut.push_back(f);
		}
	}

	return facesOut.size();
}

size_t LGScene::
get_volumes_in_rect(std::vector<Volume*>& volsOut,
					LGObject* obj,
					float xMin, float yMin, float xMax, float yMax)
{
	volsOut.clear();
	yMin = m_viewHeight - yMin;
	yMax = m_viewHeight - yMax;
	swap(yMin, yMax);

	if(obj)
	{
		GLdouble modelMat[16];
		GLdouble projMat[16];
		GLint viewport[4];

		glGetDoublev(GL_MODELVIEW_MATRIX, modelMat);
		glGetDoublev(GL_PROJECTION_MATRIX, projMat);
		glGetIntegerv(GL_VIEWPORT, viewport);

		GLdouble vx, vy, vz;
		Grid& grid = obj->grid();
		Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);
		Grid::VolumeAttachmentAccessor<ABool> aaRenderedVOL(grid, m_aRendered);
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
				vector3& pos = aaPos[vrt];
				if(PlanePointTest(plane, pos) == RPI_INSIDE){
					allIn = false;
					break;
				}
				
				GLdouble px = pos.x();
				GLdouble py = pos.y();
				GLdouble pz = pos.z();

				gluProject(px, py, pz, modelMat, projMat,
						   viewport, &vx, &vy, &vz);

				if(vx < xMin || vx > xMax || vy < yMin || vy > yMax || vz < 0){
					allIn = false;
					break;
				}
			}
			if(allIn)
				volsOut.push_back(v);
		}
	}

	return volsOut.size();
}

size_t LGScene::
get_edges_in_rect_cut(std::vector<Edge*>& edgesOut,
					LGObject* obj,
					float xMin, float yMin, float xMax, float yMax)
{
	edgesOut.clear();
	yMin = m_viewHeight - yMin;
	yMax = m_viewHeight - yMax;
	swap(yMin, yMax);

	vector3 boxMin(xMin, yMin, 0);
	vector3 boxMax(xMax, yMax, 1.);

	if(obj)
	{
		GLdouble modelMat[16];
		GLdouble projMat[16];
		GLint viewport[4];

		glGetDoublev(GL_MODELVIEW_MATRIX, modelMat);
		glGetDoublev(GL_PROJECTION_MATRIX, projMat);
		glGetIntegerv(GL_VIEWPORT, viewport);

		GLdouble vx1, vy1, vz1;
		GLdouble vx2, vy2, vz2;

		Grid& grid = obj->grid();
		Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);
		Grid::EdgeAttachmentAccessor<ABool> aaRenderedEDGE(grid, m_aRendered);
		Plane plane = near_clip_plane();
		
		for(EdgeIterator iter = grid.begin<Edge>();
			iter != grid.end<Edge>(); ++iter)
		{
			Edge* e = *iter;
			if(!aaRenderedEDGE[e])
				continue;

			Vertex* vrt1 = e->vertex(0);
			Vertex* vrt2 = e->vertex(1);
			vector3& pos1 = aaPos[vrt1];
			vector3& pos2 = aaPos[vrt2];
			
		//	at least one of the vertices has to lie in front of the clip plane
			if((PlanePointTest(plane, pos1) == RPI_INSIDE)
			   && (PlanePointTest(plane, pos2) == RPI_INSIDE))
			{
				continue;
			}
			
			GLdouble px1 = pos1.x();
			GLdouble py1 = pos1.y();
			GLdouble pz1 = pos1.z();
			GLdouble px2 = pos2.x();
			GLdouble py2 = pos2.y();
			GLdouble pz2 = pos2.z();

			gluProject(px1, py1, pz1, modelMat, projMat,
					   viewport, &vx1, &vy1, &vz1);
			gluProject(px2, py2, pz2, modelMat, projMat,
					   viewport, &vx2, &vy2, &vz2);


			if(LineBoxIntersection(vector3(vx1, vy1, vz1),
								   vector3(vx2, vy2, vz2),
								   boxMin, boxMax))
			{
				edgesOut.push_back(e);
			}
		}
	}

	return edgesOut.size();
}

size_t LGScene::
get_faces_in_rect_cut(std::vector<Face*>& facesOut,
				  LGObject* obj,
				  float xMin, float yMin, float xMax, float yMax)
{
	facesOut.clear();
	yMin = m_viewHeight - yMin;
	yMax = m_viewHeight - yMax;
	swap(yMin, yMax);

	vector3 boxMin(xMin, yMin, 0);
	vector3 boxMax(xMax, yMax, 1.);

	if(obj)
	{
		GLdouble modelMat[16];
		GLdouble projMat[16];
		GLint viewport[4];

		glGetDoublev(GL_MODELVIEW_MATRIX, modelMat);
		glGetDoublev(GL_PROJECTION_MATRIX, projMat);
		glGetIntegerv(GL_VIEWPORT, viewport);

		Grid& grid = obj->grid();
		SubsetHandler& sh = obj->subset_handler();
		Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);
		Grid::FaceAttachmentAccessor<ABool> aaRenderedFACE(grid, m_aRendered);
		Plane plane = near_clip_plane();
		
		for(int si = 0; si < sh.num_subsets(); ++si)
		{
			if(!obj->subset_is_visible(si))
				continue;

			for(FaceIterator iter = sh.begin<Face>(si);
				iter != sh.end<Face>(si); ++iter)
			{
				Face* f = *iter;
				if(!aaRenderedFACE[f])
					continue;

				assert((f->num_vertices() == 3 || f->num_vertices() == 4) && "unsupported number of vertices");

				vector3 projPos[4];
				bool oneLiesInFront = false;
			
				for(size_t i = 0; i < f->num_vertices(); ++i){
					Vertex* vrt = f->vertex(i);
					vector3& pos = aaPos[vrt];
					
				//	at least one of the vertices has to lie in front of the clip plane
					if(PlanePointTest(plane, pos) == RPI_OUTSIDE)
						oneLiesInFront = true;
											
					GLdouble px = pos.x();
					GLdouble py = pos.y();
					GLdouble pz = pos.z();
					GLdouble vx, vy, vz;

					gluProject(px, py, pz, modelMat, projMat,
							   viewport, &vx, &vy, &vz);

					projPos[i].x() = vx;
					projPos[i].y() = vy;
					projPos[i].z() = vz;
				}

				if(!oneLiesInFront)
					continue;

				bool intersecting = TriangleBoxIntersection(
										projPos[0], projPos[1], projPos[2],
										boxMin, boxMax);

				if(!intersecting && f->num_vertices() == 4){
					intersecting = TriangleBoxIntersection(
										projPos[0], projPos[2], projPos[3],
										boxMin, boxMax);
				}

				if(intersecting)
					facesOut.push_back(f);
			}
		}
	}

	return facesOut.size();
}

size_t LGScene::
get_volumes_in_rect_cut(std::vector<Volume*>& volsOut,
				  LGObject* obj,
				  float xMin, float yMin, float xMax, float yMax)
{
	volsOut.clear();
	yMin = m_viewHeight - yMin;
	yMax = m_viewHeight - yMax;
	swap(yMin, yMax);

	vector3 boxMin(xMin, yMin, 0);
	vector3 boxMax(xMax, yMax, 1.);

	if(obj)
	{
		GLdouble modelMat[16];
		GLdouble projMat[16];
		GLint viewport[4];

		glGetDoublev(GL_MODELVIEW_MATRIX, modelMat);
		glGetDoublev(GL_PROJECTION_MATRIX, projMat);
		glGetIntegerv(GL_VIEWPORT, viewport);

		Grid& grid = obj->grid();
		SubsetHandler& sh = obj->subset_handler();
		Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);
		Plane plane = near_clip_plane();
		
		grid.begin_marking();

		Grid::volume_traits::secure_container vols;

		for(FaceIterator iter = grid.begin<Face>();
			iter != grid.end<Face>(); ++iter)
		{
			Face* f = *iter;

			assert((f->num_vertices() == 3 || f->num_vertices() == 4) && "unsupported number of vertices");

			vector3 projPos[4];
			bool oneLiesInFront = false;
			for(size_t i = 0; i < f->num_vertices(); ++i){
				Vertex* vrt = f->vertex(i);
				vector3& pos = aaPos[vrt];
			//	at least one of the vertices has to lie in front of the clip plane
				if(PlanePointTest(plane, pos) == RPI_OUTSIDE)
					oneLiesInFront = true;
				
				GLdouble px = pos.x();
				GLdouble py = pos.y();
				GLdouble pz = pos.z();
				GLdouble vx, vy, vz;

				gluProject(px, py, pz, modelMat, projMat,
						   viewport, &vx, &vy, &vz);

				projPos[i].x() = vx;
				projPos[i].y() = vy;
				projPos[i].z() = vz;
			}

			if(!oneLiesInFront)
				continue;

			bool intersecting = TriangleBoxIntersection(
									projPos[0], projPos[1], projPos[2],
									boxMin, boxMax);

			if(!intersecting && f->num_vertices() == 4){
				intersecting = TriangleBoxIntersection(
									projPos[0], projPos[2], projPos[3],
									boxMin, boxMax);
			}

			if(intersecting){
			//	since the face is intersecting, associated volumes do so too.
				grid.associated_elements(vols, f);
				for(size_t ivol = 0; ivol < vols.size(); ++ivol){
					Volume* vol = vols[ivol];
					if(!grid.is_marked(vol)){
						grid.mark(vol);
						if(obj->subset_is_visible(sh.get_subset_index(vol)))
							volsOut.push_back(vol);
					}
				}
			}
		}
		grid.end_marking();
	}

	return volsOut.size();
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
set_element_draw_mode(bool drawVrts, bool drawEdges, bool drawFaces, bool drawVols)
{
	m_drawVertices = drawVrts;
	m_drawEdges = drawEdges;
	m_drawFaces = drawFaces;
	m_drawVolumes = drawVols;
}
