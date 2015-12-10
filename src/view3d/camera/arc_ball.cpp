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

CArcBall::CArcBall()
{
	MatIdentity(m_matRotation);
	quaternion_from_matrix(&m_quatRotation, &m_matRotation);
	m_bDrag = false;
}

matrix44* CArcBall::get_rotation_matrix()
{
	if(m_bDrag)
		matrix_from_quaternion(&m_matRotation, &m_quatRotation);
	return &m_matRotation;
}

CQuaternion* CArcBall::get_rotation_quaternion()
{
	return &m_quatRotation;
}

void CArcBall::set_rotation_quaternion(CQuaternion* pQuaternion)
{
	end_drag();
	m_quatRotation = (*pQuaternion);
	matrix_from_quaternion(&m_matRotation, &m_quatRotation);
}

void CArcBall::set_window(int nWidth, int nHeight, float fRadius, int OffsetX, int OffsetY)
{
	m_Screen = vector2((float)nWidth, (float)nHeight);
	m_Offset = vector2((float)OffsetX, (float)OffsetY);
	m_fRadius = fRadius;
}

void CArcBall::begin_drag(int x, int y)
{
	if(m_bDrag)
		end_drag();

	m_quatDown = m_quatRotation;
	m_vDown = get_ball_point_from_screen_coords(x, (int)m_Screen.y() - y);

	m_bDrag = true;
}

void CArcBall::drag_to(int x, int y)
{
	if(m_bDrag)
	{
		vector3 v = get_ball_point_from_screen_coords(x, (int)m_Screen.y() - y);
		m_quatRotation = m_quatDown * get_quat_from_ball_points(m_vDown, v);
	}
}

void CArcBall::end_drag()
{
	if(m_bDrag)
		matrix_from_quaternion(&m_matRotation, &m_quatRotation);
	m_bDrag = false;
}

vector3 CArcBall::get_ball_point_from_screen_coords(int nx, int ny)
{
    // Scale to screen
    float x   = ((float)nx - m_Offset.x() - m_Screen.x()/2.f) / (m_fRadius*m_Screen.x()/2.f);
    float y   = ((float)ny - m_Offset.y() - m_Screen.y()/2.f) / (m_fRadius*m_Screen.y()/2.f);

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
    return vector3( x, y, z );
}

CQuaternion CArcBall::get_quat_from_ball_points(vector3& vFrom, vector3& vTo)
{
    float fDot = Vec3Dot(vFrom, vTo);
    vector3 vPart;
    Vec3Cross(vPart, vTo, vFrom);

	CQuaternion quat(vPart.x(), vPart.y(), vPart.z(), fDot);
	quat.normalize();
    return quat;
}

}

