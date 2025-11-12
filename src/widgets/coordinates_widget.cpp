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
#include "truncated_double_spin_box.hpp"
#include "coordinates_widget.hpp"

CoordinatesWidget::
CoordinatesWidget(const QString& name, QWidget* parent,
				  ITool* tool, bool apply_on_change,
				  bool show_apply_button) :
	QFrame(parent),
	_apply_on_change(apply_on_change),
	_refreshing_coords(false)
{
	_tool = tool;
	setFrameStyle(StyledPanel | Sunken);

	auto v_layout = new QVBoxLayout(this);
	v_layout->setSpacing(2);

	auto* form_layout = new QFormLayout();
	form_layout->setSpacing(5);
	form_layout->setHorizontalSpacing(10);
	form_layout->setVerticalSpacing(8);
	v_layout->addLayout(form_layout);

	v_layout->addSpacing(15);

//	input boxes
	_x = new TruncatedDoubleSpinBox(this);
	_x->setLocale(QLocale(tr("C")));
	_x->setValue(0);
	_x->setDecimals(9);
	_x->setRange(-1e+9, 1e+9);
	_x->setSingleStep(1.);
	connect(_x, &TruncatedDoubleSpinBox::valueChanged, this, &CoordinatesWidget::valueChanged); // valueChanged(double
	form_layout->addRow(tr("x:"), _x);

	_y = new TruncatedDoubleSpinBox(this);
	_y->setLocale(QLocale(tr("C")));
	_y->setValue(0);
	_y->setDecimals(9);
	_y->setRange(-1e+9, 1e+9);
	_y->setSingleStep(1.);
	connect(_y, &TruncatedDoubleSpinBox::valueChanged, this, &CoordinatesWidget::valueChanged);// valueChanged(double
	form_layout->addRow(tr("y:"), _y);

	_z = new TruncatedDoubleSpinBox(this);
	_z->setLocale(QLocale(tr("C")));
	_z->setValue(0);
	_z->setDecimals(9);
	_z->setRange(-1e+9, 1e+9);
	_z->setSingleStep(1.);
	connect(_z, &TruncatedDoubleSpinBox::valueChanged, this, &CoordinatesWidget::valueChanged); // valueChanged(double)
	form_layout->addRow(tr("z:"), _z);

	_line_edit = new QLineEdit(this);
	_line_edit->setText(tr("0 0 0"));
	connect(_line_edit, &QLineEdit::textEdited,this, &CoordinatesWidget::textEdited); // textEdited(const QString)
	form_layout->addRow(tr("text input:"), _line_edit);

//	create apply, ok and cancel buttons
	if(show_apply_button){
		auto* btn = new QPushButton(tr("Apply"), this);
		v_layout->addWidget(btn, 0, Qt::AlignLeft);
		connect(btn, &QPushButton::clicked, this, &CoordinatesWidget::apply);
	}
}

void CoordinatesWidget::
set_coords(double x, double y, double z)
{
//todo store default coords for cancel
	_refreshing_coords = true;
	_x->setValue(x);
	_y->setValue(y);
	_z->setValue(z);
	std::stringstream ss;
	ss << x << " " << y << " " << z;
	_line_edit->setText(ss.str().c_str());
	_refreshing_coords = false;
}

double CoordinatesWidget::x() const	{return _x->value();}
double CoordinatesWidget::y() const	{return _y->value();}
double CoordinatesWidget::z() const	{return _z->value();}

void CoordinatesWidget::
valueChanged(double)
{
//	if we're not refreshing the value from the text box,
//	we'll have to update text box
	if(_refreshing_coords)
		return;

	_refreshing_coords = true;

	std::stringstream ss;
	ss << _x->value() << " " << _y->value() << " " << _z->value();

	_line_edit->setText(ss.str().c_str());

	if(_apply_on_change)
		apply();

	_refreshing_coords = false;
}

void CoordinatesWidget::
textEdited(const QString& new_text)
{
//	only refresh coordinates if we're not already doing it.
	if(_refreshing_coords)
		return;

	_refreshing_coords = true;

//	parse the coordinates
	std::stringstream ss(_line_edit->text().toStdString());
	double val;
	int coord_counter = 0;
	while(!ss.eof()){
	//	we'll ignore ' ', ',', '(', ')' '/'
		int next_char = ss.peek();
		if(next_char == ',' || next_char == '('
		  || next_char == ')' || next_char == '/'
		  || next_char == ' ')
		{
			ss.ignore(1);
			continue;
		}

		ss >> val;
		if(ss.fail())
			break;
		switch(coord_counter){
			case 0:	_x->setValue(val); break;
			case 1:	_y->setValue(val); break;
			case 2:	_z->setValue(val); break;
		}
		++coord_counter;
	}

	if(_apply_on_change)
		apply();

	_refreshing_coords = false;
}

void CoordinatesWidget::
apply()
{
	LGObject* obj = app::getActiveObject();
	if(_tool && obj){
		try{
			_tool->execute(obj, this);
		}
		catch(ug::UGError error){
			UG_LOG("Execution of tool " << _tool->get_name() << " failed with the following message:\n");
			UG_LOG("  " << error.get_msg() << std::endl);
		}
	}
}
