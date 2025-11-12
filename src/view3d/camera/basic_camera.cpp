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

CBasicCamera::CBasicCamera()
{
	reset();
}

void CBasicCamera::reset()
{
	_v_from = Vector3(0, 0, 0);
	_v_to = Vector3(0, 0, 1.f);
	_v_x = Vector3(1.f, 0, 0);
	_v_y = Vector3(0, 1.f, 0);
	_v_z = Vector3(0, 0, 1.f);
	_world_scale = Vector3(1, 1, 1);
	_f_distance = 5.f;
}

SCameraState CBasicCamera::get_camera_state()
{
	SCameraState CameraState;

	get_orientation();

	CameraState.vFrom = _v_from;
	CameraState.vTo = _v_to;
	CameraState.fDistance = Vec3Distance(_v_from, _v_to);
	CameraState.quatOrientation = _quat_orientation;

	return CameraState;
}

void CBasicCamera::set_camera_state(SCameraState& CameraState)
{
	axis_from_quaternion(&_v_x, &_v_y, &_v_z, &CameraState.quatOrientation);
	Vec3Normalize(_v_x, _v_x);
	Vec3Normalize(_v_y, _v_y);
	Vec3Normalize(_v_z, _v_z);

	_quat_orientation = CameraState.quatOrientation;
	_v_to = CameraState.vTo;
	_v_from = CameraState.vFrom;
	_f_distance = CameraState.fDistance;

	get_camera_transform();
}

void CBasicCamera::set_world_scale(const Vector3& ws)
{
	_world_scale = ws;
	for(int i = 0; i < 3; ++i){
		if(_world_scale[i] >= 0 && _world_scale[i] < SMALL)
			_world_scale[i] = SMALL;
		if(_world_scale[i] < 0 && _world_scale[i] > -SMALL)
			_world_scale[i] = -SMALL;
	}
}

const Vector3& CBasicCamera::world_scale() const
{
	return _world_scale;
}

void CBasicCamera::move_object_space(float dx, float dy, float dz)
{
	Vector3 vT, vTmp;
	Vec3Scale(vT, _v_x, dx);
	Vec3Scale(vTmp, _v_y, dy);
	Vec3Add(vT, vT, vTmp);
	Vec3Scale(vTmp, _v_z, dz);
	Vec3Add(vT, vT, vTmp);

	for(int i = 0; i < 3; ++i)
		vT[i] /= _world_scale[i];

	Vec3Add(_v_from, _v_from, vT);
	Vec3Add(_v_to, _v_to, vT);
}

void CBasicCamera::move_world_space(float dx, float dy, float dz)
{
	Vector3 vT(dx, dy, dz);
	Vec3Add(_v_from, _v_from, vT);
	Vec3Add(_v_to, _v_to, vT);
}

void CBasicCamera::rotate(CQuaternion& q, Vector3* pCenter)
{
	CQuaternion qi = q.inverse();

	CQuaternion v(_v_x.x(), _v_x.y(), _v_x.z(), 0);
	v = q * v * qi;
	_v_x = Vector3(v.x, v.y, v.z);

	v = CQuaternion(_v_y.x(), _v_y.y(), _v_y.z(), 0);
	v = (q * v) * qi;
	_v_y = Vector3(v.x, v.y, v.z);

	v = CQuaternion(_v_z.x(), _v_z.y(), _v_z.z(), 0);
	v = (q * v) * qi;
	_v_z = Vector3(v.x, v.y, v.z);

//	rotate vFrom / vTo
	v = CQuaternion(_v_from.x() - pCenter->x(), _v_from.y() - pCenter->y(), _v_from.z() - pCenter->z(), 0);
	v = (q * v) * qi;
	Vec3Add(_v_from, *pCenter, Vector3(v.x, v.y, v.z));

	v = CQuaternion(_v_to.x() - pCenter->x(), _v_to.y() - pCenter->y(), _v_to.z() - pCenter->z(), 0);
	v = (q * v) * qi;
	Vec3Add(_v_to, *pCenter, Vector3(v.x, v.y, v.z));
}

void CBasicCamera::rotate_object_space_x(float drads, Vector3* pCenter)
{
	CQuaternion q;
	q.set_values(drads, _v_x);
	q.normalize();

	CQuaternion qi = q.inverse();

	CQuaternion v(_v_y.x(), _v_y.y(), _v_y.z(), 0);
	v = (q * v) * qi;
	_v_y = Vector3(v.x, v.y, v.z);

	v = CQuaternion(_v_z.x(), _v_z.y(), _v_z.z(), 0);
	v = (q * v) * qi;
	_v_z = Vector3(v.x, v.y, v.z);

//	rotate vFrom / vTo
	v = CQuaternion(_v_from.x() - pCenter->x(), _v_from.y() - pCenter->y(), _v_from.z() - pCenter->z(), 0);
	v = (q * v) * qi;
	Vec3Add(_v_from, *pCenter, Vector3(v.x, v.y, v.z));

	v = CQuaternion(_v_to.x() - pCenter->x(), _v_to.y() - pCenter->y(), _v_to.z() - pCenter->z(), 0);
	v = (q * v) * qi;
	Vec3Add(_v_to, *pCenter, Vector3(v.x, v.y, v.z));

}

void CBasicCamera::rotate_object_space_y(float drads, Vector3* pCenter)
{
	CQuaternion q;
	q.set_values(drads, _v_y);
	q.normalize();

	CQuaternion qi = q.inverse();

	CQuaternion v(_v_x.x(), _v_x.y(), _v_x.z(), 0);
	v = q * v * qi;
	_v_x = Vector3(v.x, v.y, v.z);

	v = CQuaternion(_v_z.x(), _v_z.y(), _v_z.z(), 0);
	v = q * v * qi;
	_v_z = Vector3(v.x, v.y, v.z);

//	rotate vFrom / vTo
	v = CQuaternion(_v_from.x() - pCenter->x(), _v_from.y() - pCenter->y(), _v_from.z() - pCenter->z(), 0);
	v = (q * v) * qi;
	Vec3Add(_v_from, *pCenter, Vector3(v.x, v.y, v.z));

	v = CQuaternion(_v_to.x() - pCenter->x(), _v_to.y() - pCenter->y(), _v_to.z() - pCenter->z(), 0);
	v = (q * v) * qi;
	Vec3Add(_v_to, *pCenter, Vector3(v.x, v.y, v.z));

}

void CBasicCamera::rotate_object_space_z(float drads, Vector3* pCenter)
{
	CQuaternion q;
	q.set_values(drads, _v_z);
	q.normalize();
	CQuaternion qi = q.inverse();

	CQuaternion v(_v_x.x(), _v_x.y(), _v_x.z(), 0);
	v = q * v * qi;
	_v_x = Vector3(v.x, v.y, v.z);

	v = CQuaternion(_v_y.x(), _v_y.y(), _v_y.z(), 0);
	v = q * v * qi;
	_v_y = Vector3(v.x, v.y, v.z);

//	rotate vFrom / vTo
	v = CQuaternion(_v_from.x() - pCenter->x(), _v_from.y() - pCenter->y(), _v_from.z() - pCenter->z(), 0);
	v = (q * v) * qi;
	Vec3Add(_v_from, *pCenter, Vector3(v.x, v.y, v.z));

	v = CQuaternion(_v_to.x() - pCenter->x(), _v_to.y() - pCenter->y(), _v_to.z() - pCenter->z(), 0);
	v = (q * v) * qi;
	Vec3Add(_v_to, *pCenter, Vector3(v.x, v.y, v.z));

}

void CBasicCamera::scale_from_to(float scale, const Vector3* pCenter)
{
	Vector3 v;
	Vec3Subtract(v, _v_from, *pCenter);
	Vec3Scale(v, v, scale);
	Vec3Add(_v_from, *pCenter, v);

	Vec3Subtract(v, _v_to, *pCenter);
	Vec3Scale(v, v, scale);
	Vec3Add(_v_to, *pCenter, v);
	_f_distance = Vec3Distance(_v_from, _v_to);
}

Matrix44* CBasicCamera::get_camera_transform()
{
	Vec3Normalize(_v_x, _v_x);
	Vec3Normalize(_v_y, _v_y);
	Vec3Normalize(_v_z, _v_z);

	Matrix44 matRot(_v_x.x(), _v_y.x(), _v_z.x(), 0,
					_v_x.y(), _v_y.y(), _v_z.y(), 0,
					_v_x.z(), _v_y.z(), _v_z.z(), 0,
					0, 0, 0, 1.f);

	Matrix44 matTrans;
	MatTranslation(matTrans, -_v_from.x(), -_v_from.y(), -_v_from.z());

	Matrix44 matTmp;
	MatMultiply(matTmp, matRot, matTrans);

	Matrix44 matScale(	_world_scale.x(),	0,	0,	0,
	                  	0,	_world_scale.y(),	0,	0,
	                  	0,	0,	_world_scale.z(),	0,
	                  	0,	0,	0,	1);

	MatMultiply(_mat_transform, matScale, matTmp);
	return &_mat_transform;
}

CQuaternion* CBasicCamera::get_orientation()
{
	get_camera_transform();
	quaternion_from_matrix(&_quat_orientation, &_mat_transform);
	return &_quat_orientation;
}
/*
SCameraState calculate_camera_state(SCameraState& OldState, vector3* vFrom, vector3* vTo)
{
	SCameraState cs;

	vector3 vX, vY, vZ;
	axis_from_quaternion(&vX, &vY, &vZ, &OldState.quatOrientation);

	vector3 vDir;
	Vec3Subtract(vDir, *vFrom, *vTo);
	Vec3Normalize(vDir, vDir);
	float DotRight = Vec3Dot(vDir, vX);
	float DotUp = Vec3Dot(vDir, vY);

	if(fabs(DotUp) < fabs(DotRight))
	{
	//	first calculate m_vX = vDir x m_vY		then m_vY = vDir x m_vX

		Vec3Cross(vX, vY, vDir);
		Vec3Normalize(vX, vX);
		Vec3Cross(vY, vDir, vX);

	}
	else
	{
	//	first calculate m_vY = vDir x m_vX		then m_vX = vDir x m_vY
		Vec3Cross(vY, vDir, vX);
		Vec3Normalize(vY, vY);
		Vec3Cross(vX, vY, vDir);
	}
	vZ = vDir;

	Vec3Normalize(vX, vX);
	Vec3Normalize(vY, vY);
	Vec3Normalize(vZ, vZ);

	quaternion_from_axis(&cs.quatOrientation, &vX, &vY, &vZ);
	cs.quatOrientation.normalize();

	cs.vTo = *vTo;
	cs.vFrom = *vFrom;
	cs.fDistance = Vec3Distance(*vTo, *vFrom);

	return cs;
}*/

SCameraState CBasicCamera::calculate_camera_state(SCameraState& OldState, Vector3* vFrom, Vector3* vTo)
{//	calculates the rotation-transformation.
	SCameraState cs;

	Vector3 dirOld, dirNew;
	Vec3Subtract(dirOld, OldState.vTo, OldState.vFrom);
	Vec3Subtract(dirNew, *vTo, *vFrom);

	Vec3Normalize(dirOld, dirOld);
	Vec3Normalize(dirNew, dirNew);

	float fDot = Vec3Dot(dirOld, dirNew);
	Vector3 vPart;
	Vec3Cross(vPart, dirOld, dirNew);

	CQuaternion q1(vPart.x(), vPart.y(), vPart.z(), fDot);
	q1.normalize();

/*
	vector3 dirNew;
	Vec3Subtract(dirNew, *vTo, *vFrom);
	Vec3Normalize(dirNew, dirNew);

	vector3 vX, vY, vZ;
	axis_from_quaternion(&vX, &vY, &vZ, &OldState.quatOrientation);

//	calculate the quaternion for x rotation
//	project it into the yz plane first
	float d = Vec3Dot(vX, dirNew);
	vector3 v;
	Vec3Scale(v, vX, d);
	Vec3Subtract(v, dirNew, v);
	Vec3Normalize(v, v);
	Vec3Scale(v, v, -1);
//	now calculate the angle and the quaternion.
	d = Vec3Dot(vZ, v);
	CQuaternion q1;
	q1.set_values(vX.x(), vX.y(), vX.z(), 0);
	q1.normalize();
*/
	cs.vTo = *vTo;
	cs.vFrom = *vFrom;
	Vector3 tto;
	for(int i = 0; i < 3; ++i)
		tto[i] = (*vTo)[i] * _world_scale[i];
	cs.fDistance = Vec3Distance(tto, *vFrom);
	cs.quatOrientation = OldState.quatOrientation * q1;
	cs.quatOrientation.normalize();

	return cs;
}

SCameraState CBasicCamera::interpolate_camera_states(SCameraState& state1, SCameraState& state2, float IA)
{
	SCameraState cs;
	float w1 = 1.f - IA;
	float w2 = IA;

	Vector3 vTmp;
	Vec3Scale(cs.vFrom, state1.vFrom, w1);
	Vec3Scale(vTmp, state2.vFrom, w2);
	Vec3Add(cs.vFrom, cs.vFrom, vTmp);

	Vec3Scale(cs.vTo, state1.vTo, w1);
	Vec3Scale(vTmp, state2.vTo, w2);
	Vec3Add(cs.vTo, cs.vTo, vTmp);

	cs.fDistance = w1 * state1.fDistance + w2 * state2.fDistance;
	quaternion_slerp(&cs.quatOrientation, &state1.quatOrientation, &state2.quatOrientation, IA);
	return cs;
}

}
