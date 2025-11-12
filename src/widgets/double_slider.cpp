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

#include <algorithm>
#include <QSlider>
#include <QHBoxLayout>
#include "double_slider.hpp"

DoubleSlider::
DoubleSlider(QWidget* parent) :
	QWidget(parent),
	_value(0),
	_min(0),
	_max(1),
	_resolution(10000),
	_single_step(0.1)
{
	_slider = new QSlider(Qt::Horizontal, this);
	_slider->setRange(0, (int)_resolution);
	_slider->setValue(0);
	setSingleStep(_single_step);

	auto* l = new QHBoxLayout(this);
	l->setSpacing(0);
	l->setContentsMargins(0, 0, 0, 0);
	l->addWidget(_slider);

	connect(_slider, &QSlider::valueChanged, this, &DoubleSlider::sliderValueChanged);
}

void DoubleSlider::
setValue(double val)
{
	_value = val;
	if(_value < _min)
		_value = _min;
	else if(_value > _max)
		_value = _max;

	if(_max <= _min)
		_slider->setSliderPosition(0);
	else{
		_slider->setSliderPosition((int)(_resolution * (_value - _min)
													   / (_max - _min)));
	}
}

double DoubleSlider::
value() const
{
	return _value;
}

void DoubleSlider::
setSingleStep(double singleStep)
{
	if(singleStep < 0)
		singleStep = 0;

	_single_step = singleStep;

//	calculate percentage of range and set single-step property of
//	underlying QSlider accordingly.
	double p;
	if(_max <= _min)
		p = 0.1;
	else
		p = _single_step / (_max - _min);
	_slider->setSingleStep(std::max<int>((int)(p * _resolution), 1));
}

double DoubleSlider::
singleStep() const
{
	return _single_step;
}


void DoubleSlider::
setRange(double min, double max)
{
	_min = min;
	_max = max;
	if(_max < _min)
		_max = _min;
	setSingleStep(_single_step);
	setValue(_value);
}

double DoubleSlider::
minimum() const
{
	return _min;
}

double DoubleSlider::
maximum() const
{
	return _max;
}


void DoubleSlider::
setResolution(int resolution)
{
	if(resolution < 0)
		resolution = 0;
	_resolution = resolution;
	setSingleStep(_single_step);
	setValue(_value);
}


void DoubleSlider::
sliderValueChanged(int value)
{
	double p = (double)(value - _slider->minimum()) /
			   (double)(_slider->maximum() - _slider->minimum());
	_value = _min + p * (_max - _min);
	emit valueChanged(_value);
}
