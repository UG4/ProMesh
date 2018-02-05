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

#include "camera.h"

namespace cam
{

CModelViewerCamera::CModelViewerCamera() : CBasicCamera()
{
	m_bDragging = false;
	move_object_space(0, 0, -1);
}

SCameraState CModelViewerCamera::get_camera_state()
{
	SCameraState CameraState;

	get_camera_transform();

	CameraState.vFrom = m_vFrom;
	CameraState.vTo = m_vTo;
	CameraState.fDistance = m_fDistance;
	CameraState.quatOrientation = m_quatOrientation;

	return CameraState;
}

void CModelViewerCamera::set_camera_state(SCameraState& CameraState)
{
	axis_from_quaternion(&m_vX, &m_vY, &m_vZ, &CameraState.quatOrientation);
	Vec3Normalize(m_vX, m_vX);
	Vec3Normalize(m_vY, m_vY);
	Vec3Normalize(m_vZ, m_vZ);

	m_quatOrientation = CameraState.quatOrientation;
	m_vTo = CameraState.vTo;
	m_vFrom = CameraState.vFrom;
	m_fDistance = CameraState.fDistance;

	m_ArcBall.set_rotation_quaternion(&m_quatOrientation);
	get_camera_transform();
}

void CModelViewerCamera::begin_drag(int x, int y, unsigned int cdf)
{
	m_bDragging = true;
	m_iLastMouseX = x;
	m_iLastMouseY = y;
	m_lastCDF = cdf;
	m_ArcBall.begin_drag(x, y);
}

void CModelViewerCamera::drag_to(int x, int y, unsigned int cdf)
{
	if(m_bDragging)
	{

		bool bMoving = ((cdf & CDF_MOVE) == CDF_MOVE);
		bool bZooming = ((cdf & CDF_ZOOM) == CDF_ZOOM);

		float dx = -(float)(x - m_iLastMouseX);
		float dy = (float)(y - m_iLastMouseY);

		if(bMoving)
		{
			if(bZooming)
				move_object_space(0, 0, m_fDistance * dy / 500.f);
			else
			{
				move_object_space(m_fDistance * dx / 500.f, m_fDistance * dy / 500.f, 0);
			}
		}
		else
		{
			if(bZooming)
			{
				if(dy > 5.0)
					dy = 5.0;
				if(dy < -5.0)
					dy = -5.0;

				m_fDistance *= (1.f + dy / 30.f);
			}
		}


		if(!(bMoving || bZooming))
		{
		//	we have to check if zoom or movement was enabled before.
		//	if so there could be a gap in the rotation.
		//	This can be avoided by restarting the drag.
			if(m_lastCDF != CDF_NONE)
			{
			//	restart drag
				m_ArcBall.end_drag();
				m_ArcBall.begin_drag(x, y);
			}
			else
				m_ArcBall.drag_to(x, y);
		}

		m_iLastMouseX = x;
		m_iLastMouseY = y;
		m_lastCDF = cdf;
	}
}

void CModelViewerCamera::end_drag(int x, int y, unsigned int cdf)
{
	m_bDragging = false;
	m_ArcBall.end_drag();
}

void CModelViewerCamera::scroll(float scrollAmount, unsigned int cdf)
{
	bool bMoving = ((cdf & CDF_MOVE) == CDF_MOVE);

	if(bMoving)
	{
		move_object_space(0, 0, -m_fDistance * scrollAmount);
	}
	else
	{
		if(scrollAmount < -0.5f)
			scrollAmount = -0.5f;
		if(scrollAmount > 0.5f)
			scrollAmount = 0.5f;

		m_fDistance *= (1.f + scrollAmount);

	}
}

void CModelViewerCamera::set_window(int nWidth, int nHeight, float fRadius, int OffsetX, int OffsetY)
{
	m_ArcBall.set_window(nWidth, nHeight, fRadius, OffsetX, OffsetY);
}

matrix44* CModelViewerCamera::get_camera_transform()
{
	m_quatOrientation = *m_ArcBall.get_rotation_quaternion();
	matrix44* matRot = m_ArcBall.get_rotation_matrix();

	m_vX.x() = (*matRot)[0][0];	m_vY.x() = (*matRot)[0][1];	m_vZ.x() = -(*matRot)[0][2];
	m_vX.y() = (*matRot)[1][0];	m_vY.y() = (*matRot)[1][1];	m_vZ.y() = -(*matRot)[1][2];
	m_vX.z() = (*matRot)[2][0];	m_vY.z() = (*matRot)[2][1];	m_vZ.z() = -(*matRot)[2][2];

	Vec3Normalize(m_vX, m_vX);
	Vec3Normalize(m_vY, m_vY);
	Vec3Normalize(m_vZ, m_vZ);

	vector3 to;
	for(int i = 0; i < 3; ++i)
		to[i] = m_vTo[i] * m_worldScale[i];

	Vec3Scale(m_vFrom, m_vZ, -m_fDistance);
	Vec3Add(m_vFrom, m_vFrom, to);

	// MatTranslation(m_matTransform, -m_vFrom.x(), -m_vFrom.y(), -m_vFrom.z());

	// MatMultiply(m_matTransform, m_matTransform, *matRot);

	matrix44 matTrans;
	MatTranslation(matTrans, -m_vFrom.x(), -m_vFrom.y(), -m_vFrom.z());

	matrix44 matTmp;
	MatMultiply(matTmp, matTrans, *matRot);

	matrix44 matScale(	m_worldScale.x(),	0,	0,	0,
	                  	0,	m_worldScale.y(),	0,	0,
	                  	0,	0,	m_worldScale.z(),	0,
	                  	0,	0,	0,	1);

	MatMultiply(m_matTransform, matScale, matTmp);


	return &m_matTransform;
}

CQuaternion* CModelViewerCamera::get_orientation()
{
	m_quatOrientation = *m_ArcBall.get_rotation_quaternion();
	return m_ArcBall.get_rotation_quaternion();
}

}

