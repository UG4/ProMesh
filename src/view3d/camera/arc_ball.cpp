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

CArcBall::CArcBall()
{
	MatIdentity(_mat_rotation);
	quaternion_from_matrix(&_quat_rotation, &_mat_rotation);
	_drag = false;
}

Matrix44* CArcBall::get_rotation_matrix()
{
	if(_drag)
		matrix_from_quaternion(&_mat_rotation, &_quat_rotation);
	return &_mat_rotation;
}

CQuaternion* CArcBall::get_rotation_quaternion()
{
	return &_quat_rotation;
}

void CArcBall::set_rotation_quaternion(CQuaternion* pQuaternion)
{
	end_drag();
	_quat_rotation = (*pQuaternion);
	matrix_from_quaternion(&_mat_rotation, &_quat_rotation);
}

void CArcBall::set_window(int nWidth, int nHeight, float fRadius, int OffsetX, int OffsetY)
{
	_screen = Vector2((float)nWidth, (float)nHeight);
	_offset = Vector2((float)OffsetX, (float)OffsetY);
	_f_radius = fRadius;
}

void CArcBall::begin_drag(int x, int y)
{
	if(_drag)
		end_drag();

	_quat_down = _quat_rotation;
	_v_down = get_ball_point_from_screen_coords(x, (int)_screen.y() - y);

	_drag = true;
}

void CArcBall::drag_to(int x, int y)
{
	if(_drag)
	{
		Vector3 v = get_ball_point_from_screen_coords(x, (int)_screen.y() - y);
		_quat_rotation = _quat_down * get_quat_from_ball_points(_v_down, v);
	}
}

void CArcBall::end_drag()
{
	if(_drag)
		matrix_from_quaternion(&_mat_rotation, &_quat_rotation);
	_drag = false;
}

Vector3 CArcBall::get_ball_point_from_screen_coords(int nx, int ny)
{
    // Scale to screen
    float x   = ((float)nx - _offset.x() - _screen.x()/2.f) / (_f_radius*_screen.x()/2.f);
    float y   = ((float)ny - _offset.y() - _screen.y()/2.f) / (_f_radius*_screen.y()/2.f);

    float z   = 0.0f;
    float mag = x*x + y*y;

    if( mag > 1.0f )
    {
        float scale = 1.0f/sqrtf(mag);
        x *= scale;
        y *= scale;
    }
    else
        z = sqrtf( 1.0f - mag );

    // Return vector
    return Vector3( x, y, z );
}

CQuaternion CArcBall::get_quat_from_ball_points(Vector3& vFrom, Vector3& vTo)
{
    float fDot = Vec3Dot(vFrom, vTo);
    Vector3 vPart;
    Vec3Cross(vPart, vTo, vFrom);

	CQuaternion quat(vPart.x(), vPart.y(), vPart.z(), fDot);
	quat.normalize();
    return quat;
}

}

