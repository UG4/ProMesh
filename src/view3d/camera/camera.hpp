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

#ifndef _BASICCAMERA_H_
#define _BASICCAMERA_H_

#include "vec_math.hpp"
#include "quaternion.hpp"
#include "matrix44.hpp"
#include "arc_ball.hpp"

namespace cam
{

enum EInterpolation
{
	I_NONE,
	I_LERP,
	I_SLERP
};

struct SCameraState
{
	Vector3		vFrom;
	Vector3		vTo;
	float		fDistance;
	CQuaternion	quatOrientation;
};

enum ECameraDragFlags
{
	CDF_NONE = 0,
	CDF_MOVE = 1,
	CDF_ZOOM = 1<<1,
};


class CBasicCamera
{
	public:
		CBasicCamera();
		virtual ~CBasicCamera()	= default;

		void reset();

		virtual void begin_drag(int x, int y, unsigned int cdf)	{}	///<	cdf has to be an or-combination of consts enumerated in ECameraDragFlags.
		virtual void drag_to(int x, int y, unsigned int cdf)	{}	///<	cdf has to be an or-combination of consts enumerated in ECameraDragFlags.
		virtual void end_drag(int x, int y, unsigned int cdf)	{}	///<	cdf has to be an or-combination of consts enumerated in ECameraDragFlags.
		virtual void scroll(float scrollAmount, unsigned int cdf)	{}
		virtual SCameraState get_camera_state();
		virtual void set_camera_state(SCameraState& CameraState);

		void set_world_scale(const Vector3& ws);
		const Vector3& world_scale() const;

		void move_object_space(float dx, float dy, float dz);
		void move_world_space(float dx, float dy, float dz);

		void rotate_object_space_x(float drads, Vector3* pCenter);
		void rotate_object_space_y(float drads, Vector3* pCenter);
		void rotate_object_space_z(float drads, Vector3* pCenter);

		void rotate(CQuaternion& q, Vector3* pCenter);

		void scale_from_to(float scale, const Vector3* pCenter);

		virtual Matrix44* get_camera_transform();
		virtual CQuaternion* get_orientation();

		Vector3* get_from()		{return &_v_from;};
		Vector3* get_to()		{return &_v_to;};
		Vector3* get_dir()		{return &_v_z;};
		float get_distance()	{return _f_distance;};

		const Vector3& get_up_dir()		{return _v_y;}
		const Vector3& get_right_dir()	{return _v_x;}
		const Vector3& get_to_dir()		{return _v_z;}

		SCameraState calculate_camera_state(SCameraState& OldState, Vector3* vFrom, Vector3* vTo);
		SCameraState interpolate_camera_states(SCameraState& state1, SCameraState& state2, float IA);
		
	protected:

		Matrix44	_mat_transform;
		CQuaternion	_quat_orientation;

		Vector3 _v_from;
		Vector3 _v_to;

		float	_f_distance;

		Vector3 _v_x;	//strafe dir
		Vector3 _v_y;	//up dir
		Vector3 _v_z;	//look dir
		Vector3 _world_scale;
};

class CModelViewerCamera : public CBasicCamera {
	public:
		CModelViewerCamera();

		~CModelViewerCamera() override = default;

		void begin_drag(int x, int y, unsigned int cdf) override;///<	cdf has to be an or-combination of consts enumerated in ECameraDragFlags.
		void drag_to(int x, int y, unsigned int cdf) override;///<	cdf has to be an or-combination of consts enumerated in ECameraDragFlags.
		void end_drag(int x, int y, unsigned int cdf) override;///<	cdf has to be an or-combination of consts enumerated in ECameraDragFlags.
		void scroll(float scrollAmount, unsigned int cdf) override;

		bool dragging()	{return _dragging;}
		SCameraState get_camera_state() override;
		void set_camera_state(SCameraState& CameraState) override;

		void set_window(int nWidth, int nHeight, float fRadius = 0.9f, int OffsetX = 0, int OffsetY = 0);

		Matrix44* get_camera_transform() override;
		CQuaternion* get_orientation() override;

	protected:
		CArcBall _arc_ball;

//		MovementControl
		bool _last_cdf;
		bool _dragging;
		int _i_last_mouse_x;
		int _i_last_mouse_y;
};

}
#endif