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

#include "camera.hpp"

namespace cam
{

CModelViewerCamera::CModelViewerCamera()
{
	_dragging = false;
	move_object_space(0, 0, -1);
}

SCameraState CModelViewerCamera::get_camera_state()
{
	SCameraState CameraState;

	get_camera_transform();

	CameraState.vFrom = _v_from;
	CameraState.vTo = _v_to;
	CameraState.fDistance = _f_distance;
	CameraState.quatOrientation = _quat_orientation;

	return CameraState;
}

void CModelViewerCamera::set_camera_state(SCameraState& CameraState)
{
	axis_from_quaternion(&_v_x, &_v_y, &_v_z, &CameraState.quatOrientation);
	Vec3Normalize(_v_x, _v_x);
	Vec3Normalize(_v_y, _v_y);
	Vec3Normalize(_v_z, _v_z);

	_quat_orientation = CameraState.quatOrientation;
	_v_to = CameraState.vTo;
	_v_from = CameraState.vFrom;
	_f_distance = CameraState.fDistance;

	_arc_ball.set_rotation_quaternion(&_quat_orientation);
	get_camera_transform();
}

void CModelViewerCamera::begin_drag(int x, int y, unsigned int cdf)
{
	_dragging = true;
	_i_last_mouse_x = x;
	_i_last_mouse_y = y;
	_last_cdf = cdf;
	_arc_ball.begin_drag(x, y);
}

void CModelViewerCamera::drag_to(int x, int y, unsigned int cdf)
{
	if(_dragging)
	{

		bool bMoving = ((cdf & CDF_MOVE) == CDF_MOVE);
		bool bZooming = ((cdf & CDF_ZOOM) == CDF_ZOOM);

		float dx = -(float)(x - _i_last_mouse_x);
		float dy = (float)(y - _i_last_mouse_y);

		if(bMoving)
		{
			if(bZooming)
				move_object_space(0, 0, _f_distance * dy / 500.f);
			else
			{
				move_object_space(_f_distance * dx / 500.f, _f_distance * dy / 500.f, 0);
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

				_f_distance *= (1.f + dy / 30.f);
			}
		}


		if(!(bMoving || bZooming))
		{
		//	we have to check if zoom or movement was enabled before.
		//	if so there could be a gap in the rotation.
		//	This can be avoided by restarting the drag.
			if(_last_cdf != CDF_NONE)
			{
			//	restart drag
				_arc_ball.end_drag();
				_arc_ball.begin_drag(x, y);
			}
			else
				_arc_ball.drag_to(x, y);
		}

		_i_last_mouse_x = x;
		_i_last_mouse_y = y;
		_last_cdf = cdf;
	}
}

void CModelViewerCamera::end_drag(int x, int y, unsigned int cdf)
{
	_dragging = false;
	_arc_ball.end_drag();
}

void CModelViewerCamera::scroll(float scrollAmount, unsigned int cdf)
{
	bool bMoving = ((cdf & CDF_MOVE) == CDF_MOVE);

	if(bMoving)
	{
		move_object_space(0, 0, -_f_distance * scrollAmount);
	}
	else
	{
		if(scrollAmount < -0.5f)
			scrollAmount = -0.5f;
		if(scrollAmount > 0.5f)
			scrollAmount = 0.5f;

		_f_distance *= (1.f + scrollAmount);

	}
}

void CModelViewerCamera::set_window(int nWidth, int nHeight, float fRadius, int OffsetX, int OffsetY)
{
	_arc_ball.set_window(nWidth, nHeight, fRadius, OffsetX, OffsetY);
}

Matrix44* CModelViewerCamera::get_camera_transform()
{
	_quat_orientation = *_arc_ball.get_rotation_quaternion();
	Matrix44* matRot = _arc_ball.get_rotation_matrix();

	_v_x.x() = (*matRot)[0][0];	_v_y.x() = (*matRot)[0][1];	_v_z.x() = -(*matRot)[0][2];
	_v_x.y() = (*matRot)[1][0];	_v_y.y() = (*matRot)[1][1];	_v_z.y() = -(*matRot)[1][2];
	_v_x.z() = (*matRot)[2][0];	_v_y.z() = (*matRot)[2][1];	_v_z.z() = -(*matRot)[2][2];

	Vec3Normalize(_v_x, _v_x);
	Vec3Normalize(_v_y, _v_y);
	Vec3Normalize(_v_z, _v_z);

	Vector3 to;
	for(int i = 0; i < 3; ++i)
		to[i] = _v_to[i] * _world_scale[i];

	Vec3Scale(_v_from, _v_z, -_f_distance);
	Vec3Add(_v_from, _v_from, to);

	// MatTranslation(m_matTransform, -m_vFrom.x(), -m_vFrom.y(), -m_vFrom.z());

	// MatMultiply(m_matTransform, m_matTransform, *matRot);

	Matrix44 matTrans;
	MatTranslation(matTrans, -_v_from.x(), -_v_from.y(), -_v_from.z());

	Matrix44 matTmp;
	MatMultiply(matTmp, matTrans, *matRot);

	Matrix44 matScale(	_world_scale.x(),	0,	0,	0,
	                  	0,	_world_scale.y(),	0,	0,
	                  	0,	0,	_world_scale.z(),	0,
	                  	0,	0,	0,	1);

	MatMultiply(_mat_transform, matScale, matTmp);


	return &_mat_transform;
}

CQuaternion* CModelViewerCamera::get_orientation()
{
	_quat_orientation = *_arc_ball.get_rotation_quaternion();
	return _arc_ball.get_rotation_quaternion();
}

}

