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

#ifndef __H__LGMATH__PLANE_SPHERE__
#define __H__LGMATH__PLANE_SPHERE__

#include "common/math/ugmath.h"

namespace ug
{
////////////////////////////////////////////////////////////////////////
//	Sphere
///	a simple sphere, described by a center and a radius.
class Sphere3
{
	public:
		Sphere3() = default;
		Sphere3(const vector3& center, number radius) : _center(center), _radius(radius)	{}

		const vector3& get_center() const		{return _center;}
		void set_center(const vector3& center)	{_center = center;}
		number get_radius() const				{return _radius;}
		void set_radius(number radius)			{_radius = radius;}

	protected:
		vector3	_center;
		number	_radius;
};

////////////////////////////////////////////////////////////////////////
//	Plane
///	a simple plane, described by a point (p) and a normal (n)
/**
 *	n is normized automatically.
 */
class Plane
{
	public:
		Plane()	= default;
		Plane(const vector3& p, const vector3& n)
			{
				set_p(p);
				set_n(n);
			}

		const vector3& get_p() const		{return _p;}

		void set_p(const vector3& p)
			{
				_p = p;
				_equ = vector4(_n.x(), _n.y(), _n.z(), -VecDot(_n, _p));
			}

		const vector3& get_n() const		{return _n;}

		void set_n(const vector3& n)
			{
				VecNormalize(_n, n);
				_equ = vector4(_n.x(), _n.y(), _n.z(), -VecDot(_n, _p));
			}

	///	equation: equ.x*x + equ.y*y + equ.z*z + equ.w() = 0
		const vector4& get_equation() const 	{return _equ;}

	protected:
		vector3 _p;
		vector3 _n;
		vector4 _equ;
};

////////////////////////////////////////////////////////////////////////
//	RelativePositionIndicator
///	used to classify the position of one object relative to another.
enum RelativePositionIndicator
{
	RPI_INSIDE = -2,
	RPI_INSIDE_TOUCHES = -1,
	RPI_CUT = 0,
	RPI_OUTSIDE_TOUCHES = 1,
	RPI_OUTSIDE = 2
};

////////////////////////////////////////////////////////////////////////
//	PlaneSphereTest
///	returns whether a sphere lies behind (inside), in front (outside) a plane or cuts it.
RelativePositionIndicator PlaneSphereTest(const Plane& plane, const Sphere3& sphere);

////////////////////////////////////////////////////////////////////////
//	PlanePointTest
///	returns whether a point lies behind (inside), in front (outside) a plane or lies on it (cuts it).
RelativePositionIndicator PlanePointTest(const Plane& plane, const vector3& point);

////////////////////////////////////////////////////////////////////////
//	PlanePointDistance
///	returns the distance of a point to a plane
number PlanePointDistance(const Plane& plane, const vector3& point);
}//	end of namespace

#endif
