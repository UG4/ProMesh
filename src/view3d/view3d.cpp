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

#include <QtWidgets>
#include <iostream>
#include "gl_includes.hpp"
#include "view3d.hpp"
#include "renderer3d_interface.hpp"

using namespace std;

View3D::View3D(QWidget *parent) : QOpenGLWidget(parent),
	_ortho_perspective(false)
{
	_view_width = 100;
	_view_height = 100;
	_aspect_ratio = 1.f;
	_fovy = 30;
	_z_near = 0.01;
	_z_far = 1000.f;

	_b_draw_sel_rect = false;

	_renderer = nullptr;

	QSurfaceFormat format;
	format.setDepthBufferSize(24);   // DepthBuffer
	format.setSwapBehavior(QSurfaceFormat::DoubleBuffer); // DoubleBuffer
	setFormat(format);

	glViewport(0, 0, _view_width, _view_height);

	this->setFocusPolicy(Qt::StrongFocus);

	_cam_interp_duration = 250.0f;
	//m_drawMode = DM_SOLID;
	_bg_color = Qt::black;

//	create the timer that is used during camera interpolation.
	_p_timer = new QTimer(this);
	connect(_p_timer, &QTimer::timeout, this, &View3D::interpolate_cam_states);
}


void View3D::set_renderer(IRenderer3D* renderer)
{
	_renderer = renderer;
	if(_renderer)
		_renderer->set_perspective(_fovy, _view_width, _view_height, _z_near, _z_far);
	update();
}

void View3D::set_background_color(const QColor& color)
{
	_bg_color = color;
	update();
}

void View3D::set_projection(float fovy, float z_near, float z_far)
{
	_fovy = fovy;
	_z_near = z_near;
	_z_far = z_far;

	if(_renderer)
		_renderer->set_perspective(_fovy, _view_width, _view_height, _z_near, _z_far);
	update();
}

void View3D::initializeGL()
{
}

void View3D::resizeGL(int width, int height)
{
	_view_width = width;
	_view_height = height;

	glViewport(0, 0, width, height);
	//ð glMatrixMode(GL_PROJECTION);
	//ð glLoadIdentity();
	_aspect_ratio = static_cast<float>(width) / static_cast<float>(height);
	//ð gluPerspective(m_fovy, m_aspectRatio, m_zNear, m_zFar);
	_camera.set_window(width, height);
	//glMatrixMode(GL_MODELVIEW);
	if(_renderer)
		_renderer->set_perspective(_fovy, _view_width, _view_height, _z_near, _z_far);
}

void View3D::paintGL()
{
//	setup gl

	glClearColor(_bg_color.redF(), _bg_color.greenF(), _bg_color.blueF(), _bg_color.alphaF());
	glShadeModel(GL_FLAT);
	glEnable(GL_DEPTH_TEST);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	cam::matrix mat = *_camera.get_camera_transform();

	if(_renderer)
	{
	//	update near and far clipping planes
		const cam::Vector3* v_from = _camera.get_from();
		const cam::Vector3* v_to = _camera.get_to();
		cam::Vector3 cam_dir = _camera.get_to_dir();
		cam::Vector3 cam_up = _camera.get_up_dir();

		_renderer->set_camera_parameters(v_from->x(), v_from->y(), v_from->z(),
										   cam_dir.x(), cam_dir.y(), cam_dir.z(),
										   cam_up.x(), cam_up.y(), cam_up.z());

		const cam::Vector3& ws = _camera.world_scale();
		_renderer->set_world_scale(ws.x(), ws.y(), ws.z());

		_renderer->get_clip_distance_estimate(_z_near, _z_far,
												v_from->x(), v_from->y(), v_from->z(),
												v_to->x(), v_to->y(), v_to->z());

		_renderer->set_transform(reinterpret_cast<float *>(&mat));

		if(!_ortho_perspective){
			_renderer->set_perspective(_fovy, _view_width, _view_height, _z_near, _z_far);
		}
		else{
		//todo: This is only a very rough implentation and is in no way ready for broad use
			float zoom = VecDistance(*v_from, *v_to);
			float aspect_inv = float(_view_width) / float(_view_height);
			// float fromDist = VecLength(*vFrom);
			UG_LOG("zoom: " << zoom << endl);
			_renderer->set_ortho_perspective(-zoom*aspect_inv, zoom*aspect_inv,
											   -zoom, zoom, -100, 100);
		}

	//	draw the scene
		_renderer->draw();

	//	draw the coordinate system
	//	projection matrix for the coordinate system in the lower left corner
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, 100.f, 0, 100.f / _aspect_ratio, 0.01, 100);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
	//	apply camera orientation only
		cam::CQuaternion* pQOrientation = _camera.get_orientation();
		cam::matrix matOrientation;
		matrix_from_quaternion(&matOrientation, pQOrientation);

		cam::matrix axMat;
		MatTranslation(axMat, 6, 6, -6);

		//cam::MatTranspose(mat, mat);
		MatMultiply(axMat, matOrientation, axMat);
		glMultMatrixf(reinterpret_cast<GLfloat *>(&axMat));
//
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glEnable(GL_LINE_SMOOTH);
		glLineWidth(2.f);
		glPointSize(2.f);
		glDisable(GL_LIGHTING);


//
//	//	render text
		constexpr GLfloat axlen = 5;
//CAUSES PROBLEMS ON SOME SYSTEMS (mscherer, mknodel)
		glColor3f(1, 0, 0);
		QPainter painter(this);
		painter.drawText(axlen, 0, "X");
		glColor3f(0, 1, 0);
		painter.drawText(0, axlen, "Y");
		glColor3f(0, 0, 1);
		painter.drawText(0, 0, "Z"); // todo (ø) axlen import to qt6 in a  cleaner way

		glBegin(GL_LINES);
		//	x
			glColor3f(1, 0, 0);
			glVertex3f(0, 0, 0);
			glVertex3f(axlen, 0, 0);
		//	y
			glColor3f(0, 1, 0);
			glVertex3f(0, 0, 0);
			glVertex3f(0, axlen, 0);
		//	z
			glColor3f(0, 0, 1);
			glVertex3f(0, 0, 0);
			glVertex3f(0, 0, axlen);
		glEnd();

	//	draw selection rect
		if(_b_draw_sel_rect){
			glDisable(GL_DEPTH_TEST);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, _view_width, 0, _view_height, -1, 1);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			glColor3f(0, 0, 0);
			glBegin(GL_POINTS);
			glVertex3f(_sel_rect_min.x(), _sel_rect_min.y(), 0);
			glVertex3f(_sel_rect_max.x(), _sel_rect_min.y(), 0);
			glVertex3f(_sel_rect_max.x(), _sel_rect_max.y(), 0);
			glVertex3f(_sel_rect_min.x(), _sel_rect_max.y(), 0);
			glEnd();

			glBegin(GL_LINES);
			glVertex3f(_sel_rect_min.x(), _sel_rect_min.y(), 0);
			glVertex3f(_sel_rect_max.x(), _sel_rect_min.y(), 0);
			glVertex3f(_sel_rect_max.x(), _sel_rect_min.y(), 0);
			glVertex3f(_sel_rect_max.x(), _sel_rect_max.y(), 0);
			glVertex3f(_sel_rect_max.x(), _sel_rect_max.y(), 0);
			glVertex3f(_sel_rect_min.x(), _sel_rect_max.y(), 0);
			glVertex3f(_sel_rect_min.x(), _sel_rect_max.y(), 0);
			glVertex3f(_sel_rect_min.x(), _sel_rect_min.y(), 0);
			glEnd();
		}

		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();

//	//	reset renderer matrix
//		m_pRenderer->set_transform((float*)&mat);
//		m_pRenderer->set_perspective(m_fovy, m_aspectRatio,
//									 m_zNear, m_zFar);
	}

}

void  View3D::
drawSelectionRect(bool bDrawIt, float xMin, float yMin,
				  float xMax, float yMax)
{
	_b_draw_sel_rect = bDrawIt;
	_sel_rect_min = cam::Vector2(xMin, _view_height - yMin);
	_sel_rect_max = cam::Vector2(xMax, _view_height - yMax);
}

unsigned int View3D::get_camera_drag_flags()
{
	const Qt::KeyboardModifiers keys = QApplication::keyboardModifiers();
	const bool shiftPressed = bool(keys & Qt::ShiftModifier);
	const bool ctrlPressed = bool(keys & Qt::ControlModifier);
	unsigned int dragFlags = cam::CDF_NONE;
	
	if(shiftPressed)
		dragFlags |= cam::CDF_ZOOM;
	if(ctrlPressed)
		dragFlags |= cam::CDF_MOVE;

	return dragFlags;
}

void View3D::fly_to(const cam::Vector3& destTo, float distance)
{
//	fly and zoom as required.
	_cs_old = _camera.get_camera_state();
	_cs_new = _camera.get_camera_state();
	_cs_new.vTo = destTo;
	_cs_new.fDistance = distance;

	// cam::vector3 v;
	// VecSubtract(v, m_csOld.vFrom, m_csOld.vTo);
	// VecNormalize(v, v);
	// VecScale(v, v, distance);
	// VecAdd(m_csNew.vFrom, m_csNew.vTo, v);

	start_interpolation();
}

void View3D::fly_to(const cam::Vector3& destTo)
{
	fly_to (destTo, _camera.get_camera_state().fDistance);
}

void  View3D::
get_ray(cam::Vector3& vFromOut, cam::Vector3& vToOut,
		float screenX, float screenY)
{
	QSize winSize = size()*this->windowHandle()->devicePixelRatio();
	cam::Matrix44 tMat = *_camera.get_camera_transform();
	GLdouble modelMat[16];
	GLdouble projMat[16];
	GLint viewport[4];

	for(int i = 0; i < 4; ++i)
	{
			for(int j = 0; j < 4; ++j)
					modelMat[4*i + j] = tMat[i][j];
	}

	glGetDoublev(GL_PROJECTION_MATRIX, projMat);
	glGetIntegerv(GL_VIEWPORT, viewport);

	GLdouble vx, vy, vz;
	gluUnProject(screenX, winSize.height() - screenY, 0,
				 (GLdouble*)modelMat, projMat, viewport,
				 &vx, &vy, &vz);

	vFromOut = cam::Vector3(vx, vy, vz);

	gluUnProject(screenX, winSize.height() - screenY, 1,
				 (GLdouble*)modelMat, projMat, viewport,
				 &vx, &vy, &vz);

	vToOut = cam::Vector3(vx, vy, vz);
}

bool View3D::
get_ray_to_geometry(cam::Vector3& v_from_out,
					 cam::Vector3& vToOut,
					 float screenX, float screen_y)
{
	QSize win_size = size()*this->devicePixelRatioF();

	GLfloat depthVal;
	glReadPixels(screenX, win_size.height() - screen_y, 1, 1,
				GL_DEPTH_COMPONENT, GL_FLOAT, &depthVal);

	cam::Matrix44 tMat = *_camera.get_camera_transform();
	GLdouble model_mat[16];
	GLdouble proj_mat[16];
	GLint viewport[4];

	for(int i = 0; i < 4; ++i){
		// UG_LOG("dbg - trans: ");
		for(int j = 0; j < 4; ++j){
			// UG_LOG(tMat[i][j] << ", ");
			model_mat[4*i + j] = tMat[i][j];
		}
		// UG_LOG(std::endl);
	}

	glGetDoublev(GL_PROJECTION_MATRIX, proj_mat);
	glGetIntegerv(GL_VIEWPORT, viewport);


	// for(int i = 0; i < 4; ++i){
	// 	UG_LOG("dbg - proj: ");
	// 	for(int j = 0; j < 4; ++j){
	// 		UG_LOG(projMat[4*i + j] << ", ");
	// 	}
	// 	UG_LOG(std::endl);
	// }

	GLdouble vx, vy, vz;
	gluUnProject(screenX, win_size.height() - screen_y, 0,
				 (GLdouble*)model_mat, proj_mat, viewport,
				 &vx, &vy, &vz);

	v_from_out = cam::Vector3(vx, vy, vz);

	gluUnProject(screenX, win_size.height() - screen_y, depthVal,
				 (GLdouble*)model_mat, proj_mat, viewport,
				 &vx, &vy, &vz);

	vToOut = cam::Vector3(vx, vy, vz);

	if(depthVal > 0.999999)
		return false;
	return true;
}

void View3D::refocus_by_screen_coords(int screen_x, int screen_y)
{
	QSize win_size = size()*this->devicePixelRatioF();

	GLfloat depth_val;
	glReadPixels(screen_x, win_size.height() - screen_y, 1, 1,
				GL_DEPTH_COMPONENT, GL_FLOAT, &depth_val);

	if(depth_val < 1.f)
	{
		cam::Matrix44 t_mat = *_camera.get_camera_transform();
		GLdouble model_mat[16];
		GLdouble proj_mat[16];
		GLint viewport[4];
		GLdouble vx, vy, vz;

		for(int i = 0; i < 4; ++i)
		{
				for(int j = 0; j < 4; ++j)
						model_mat[4*i + j] = t_mat[i][j];
		}

		glGetDoublev(GL_PROJECTION_MATRIX, proj_mat);
		glGetIntegerv(GL_VIEWPORT, viewport);

		gluUnProject(screen_x, win_size.height() - screen_y, depth_val,
					 (GLdouble*)model_mat, proj_mat, viewport, &vx, &vy, &vz);

	//	set new camera state:
		//m_csFrom = m_camera.get_camera_state();
		cam::Vector3 nvTo(vx, vy, vz);
		// cam::vector3 nvTo(vx * m_camera.world_scale().x(),
		//                   vy * m_camera.world_scale().y(),
		//                   vz * m_camera.world_scale().z());
		// UG_LOG(">>>\n");
		// UG_LOG("old from at: " << *m_camera.get_from() << endl);
		// UG_LOG("old to at: " << *m_camera.get_to() << endl);
		UG_LOG("new focus at: " << nvTo << endl);

		_cs_old = _camera.get_camera_state();
		_cs_new = _cs_old;
		_cs_new.vTo = nvTo;
		// m_csNew = m_camera.calculate_camera_state(m_csOld, m_camera.get_from(), &nvTo);
		start_interpolation();
	}
}

void View3D::start_interpolation()
{
//	starts the timer and resets m_interpDur
	_time.start();
	_p_timer->start(20);
}

void View3D::interpolate_cam_states()
{
	float ia = static_cast<float>(_time.elapsed()) / _cam_interp_duration;
	if(ia > 1.0f)
	{
		ia = 1.0f;
		_p_timer->stop();
	}

	cam::SCameraState cs = _camera.interpolate_camera_states(_cs_old, _cs_new, ia);
	_camera.set_camera_state(cs);
	update();
}

////////////////////////////////////////////////////////////////////////
//	event handlers
void View3D::mousePressEvent(QMouseEvent *event)
{
	auto scaled_event = new QMouseEvent(
		QEvent::MouseButtonPress,
		QPointF(event->position().x() * this->devicePixelRatioF(),
				event->position().y() * this->devicePixelRatioF()),
		event->globalPosition(),
		event->button(),
		event->buttons(),
		event->modifiers());


//	if alt is pressed, we'll refocus the clicked geometry.
//	if not, we'll start dragging.
	if(scaled_event->button() == Qt::LeftButton){
		_camera.begin_drag(scaled_event->position().x(), scaled_event->position().y(),
							get_camera_drag_flags());
	}
	else if(scaled_event->button() == Qt::MouseButton::MiddleButton){
		_camera.begin_drag(scaled_event->position().x(), scaled_event->position().y(), cam::CDF_MOVE);
	}

//	emit mousePress
	emit mousePressed(scaled_event);
}

void View3D::mouseMoveEvent(QMouseEvent *event)
{
	auto* scaledEvent = new QMouseEvent(QEvent::MouseButtonPress,
									QPoint(event->x()*this->devicePixelRatioF(),event->y()*this->devicePixelRatioF()),
									event->button(), event->buttons(), event->modifiers());

	unsigned int cdf;
	if(scaledEvent->buttons().testFlag(Qt::MouseButton::MiddleButton))
		cdf = cam::CDF_MOVE;
	else
		cdf = get_camera_drag_flags();

	if(_camera.dragging())
	{
		_camera.drag_to(scaledEvent->x(), scaledEvent->y(), cdf);
		update();
	}

	emit mouseMoved(scaledEvent);
}

void View3D::mouseReleaseEvent(QMouseEvent *event)
{
	auto* scaledEvent = new QMouseEvent(QEvent::MouseButtonPress,
									QPoint(event->x()*this->devicePixelRatioF(),event->y()*this->devicePixelRatioF()),
									event->button(), event->buttons(), event->modifiers());

	unsigned int cdf;
	if(scaledEvent->button() == Qt::MouseButton::MiddleButton)
		cdf = cam::CDF_MOVE;
	else
		cdf = get_camera_drag_flags();

	if(_camera.dragging())
	{
		_camera.end_drag(scaledEvent->x(), scaledEvent->y(), cdf);
		update();
	}
	emit mouseReleased(scaledEvent);
}

void View3D::wheelEvent(QWheelEvent *event)
{
	float num_degrees = event->angleDelta().y() / 8.0f;
	float numSteps = num_degrees / 15.0f;
    _camera.scroll(-numSteps * 0.1f, get_camera_drag_flags());
	event->accept();
	update();
}

void View3D::mouseDoubleClickEvent(QMouseEvent *event)
{
	auto* scaledEvent = new QMouseEvent(QEvent::MouseButtonPress,
									QPoint(event->x()*this->devicePixelRatioF(),event->y()*this->devicePixelRatioF()),
									event->button(), event->buttons(), event->modifiers());

	refocus_by_screen_coords(scaledEvent->x(), scaledEvent->y());
}

void View3D::keyReleaseEvent(QKeyEvent * event)
{
	QOpenGLWidget::keyReleaseEvent(event);
	emit keyReleased(event);
}
