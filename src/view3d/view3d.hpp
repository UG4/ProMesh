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

#ifndef __H__VIEW3D__
#define __H__VIEW3D__

#include <QOpenGLWidget>
//ð #include <QTime>
//ð #include <QColor>
#include "camera/camera.hpp"
#include <QElapsedTimer>


//	predeclarations
class IRenderer3D;
class QTimer;
class QTime;



class View3D : public QOpenGLWidget {
	Q_OBJECT
	
	public:
		explicit View3D(QWidget *parent = nullptr);
		~View3D() override = default;

	///	set the renderer.
		virtual void set_renderer(IRenderer3D* renderer);

	///	set projection parameters
		virtual void set_projection(float fovy, float z_near, float z_far);

	///	set background color
		void set_background_color(const QColor& color);

		cam::CModelViewerCamera& camera() {return _camera;}
		const cam::CModelViewerCamera& camera() const {return _camera;}

		void fly_to(const cam::Vector3& destTo, float distance);
		void fly_to(const cam::Vector3& destTo);

	///	calculated the ray from the screen-position to the back-plane.
		void get_ray(cam::Vector3& vFromOut, cam::Vector3& vToOut,
					float screenX, float screenY);

	///	calculated the ray from the screen-position to the geometry.

	/**	if the geometry is hit, true is returned and vToOut approximately
	 *	contains the intersection point.
	 *	If no intersection has been found, vToOut will be located at the
	 *	far clipping plane.*/
		bool get_ray_to_geometry(cam::Vector3& v_from_out,
								 cam::Vector3& vToOut,
								 float screenX, float screen_y);

	///	if bDrawIt is true, the view will draw a the given rect until the method is called with bDrawIt == false.
		void drawSelectionRect(bool bDrawIt, float xMin = 0, float yMin = 0,
								 float xMax = 0, float yMax = 0);
	signals:
		void mousePressed(QMouseEvent* event);
		void mouseMoved(QMouseEvent* event);
		void mouseReleased(QMouseEvent* event);
		void keyReleased(QKeyEvent* event);

	protected:
	//	derived from QGLWidget
		void initializeGL() override;
		void resizeGL(int width, int height) override;
		void paintGL() override;
		void mousePressEvent(QMouseEvent *event) override;
		void mouseMoveEvent(QMouseEvent *event) override;
		void mouseReleaseEvent(QMouseEvent *event) override;
		void wheelEvent(QWheelEvent *event) override;
		void mouseDoubleClickEvent(QMouseEvent *event) override;
		void keyReleaseEvent(QKeyEvent * event) override;

	//	helper methods
		unsigned int get_camera_drag_flags();
		void refocus_by_screen_coords(int screen_x, int screen_y);
		void start_interpolation();

	//	slots
	protected slots:
		void interpolate_cam_states();

	protected:
	//	camera
		cam::CModelViewerCamera	_camera;
		float _fovy;
		float _aspect_ratio;
		float _z_near;
		float _z_far;
		int _view_width;
		int _view_height;
		bool _ortho_perspective;

	//	camera interpolation
		cam::SCameraState _cs_old;
		cam::SCameraState _cs_new;
		float _cam_interp_duration; //[ms] duration of interpolation since start.

		IRenderer3D* _renderer;
		QColor _bg_color;
		QTimer* _p_timer;
		QElapsedTimer _time;
		
	//	selection rect
		bool _b_draw_sel_rect;
		cam::Vector2 _sel_rect_min;
		cam::Vector2 _sel_rect_max;
};

#endif
