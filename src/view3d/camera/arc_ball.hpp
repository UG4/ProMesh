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

#ifndef __H__CAM__ARC_BALL__
#define __H__CAM__ARC_BALL__

#include "vec_math.hpp"
#include "quaternion.hpp"
#include "matrix44.hpp"

namespace cam
{

class CArcBall
{
	public:
		CArcBall();
		void set_window(int nWidth, int nHeight, float fRadius = 0.9f, int OffsetX = 0, int OffsetY = 0);

		Matrix44* get_rotation_matrix();
		CQuaternion* get_rotation_quaternion();
		void set_rotation_quaternion(CQuaternion* pQuaternion);

		void begin_drag(int x, int y);
		void drag_to(int x, int y);
		void end_drag();

	private:
		Vector3 get_ball_point_from_screen_coords(int nx, int ny);
		CQuaternion get_quat_from_ball_points(Vector3& vFrom, Vector3& vTo);

		bool _drag;

		Matrix44 _mat_rotation;
		CQuaternion _quat_rotation;

		CQuaternion _quat_down;
		Vector3 _v_down;

		Vector2 _screen;
		Vector2 _offset;

		float _f_radius;

		int _i_mouse_last_x;
		int _i_mouse_last_y;
};

}//	end of namespace

#endif
