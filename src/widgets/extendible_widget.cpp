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
 
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QLabel>
#include "extendible_widget.hpp"


ExtendibleWidget::ExtendibleWidget(QWidget* parent) :
	QFrame(parent),
	_widget(nullptr)
{
	auto* mainLayout = new QVBoxLayout(this);
	_v_layout = new QVBoxLayout();
	auto* hLayout = new QHBoxLayout();

	mainLayout->setSpacing(0);
	mainLayout->setContentsMargins(0, 0, 10, 0);
	hLayout->setSpacing(0);
	hLayout->setContentsMargins(0, 0, 0, 0);
	_v_layout->setContentsMargins(10, 0, 0, 0);

	this->setLayout(mainLayout);
	mainLayout->addLayout(hLayout);
	mainLayout->addLayout(_v_layout);

	_toolButton = new QToolButton(this);
	_toolButton->setCheckable(true);
	// m_toolButton->setArrowType(Qt::RightArrow);
	hLayout->addWidget(_toolButton);

	auto* header = new ExtendibleWidgetHeader(this);
	connect(header, &ExtendibleWidgetHeader::clicked, this, &ExtendibleWidget::toggle);
	connect(header, &ExtendibleWidgetHeader::double_clicked, this, &ExtendibleWidget::toggle);

	_header = header;
	hLayout->addWidget(_header);

	connect(_toolButton, &QToolButton::toggled, this, &ExtendibleWidget::setChecked);
}




void ExtendibleWidget::setText(const QString& text)
{
	_header->setText(text);
}

void ExtendibleWidget::setInfoText(const QString& toolTip)
{
	_header->setToolTip(toolTip);
	this->setToolTip(toolTip);
}

void ExtendibleWidget::setWidget(QWidget* widget)
{
	if(_widget){
		_v_layout->removeWidget(_widget);
	}
	_widget = widget;
	_v_layout->addWidget(_widget);
	if(_toolButton->isChecked())
		_widget->show();
	else
		_widget->hide();
}

void ExtendibleWidget::setChecked(bool checked)
{
	if(_toolButton->isChecked() != checked){
		_toolButton->setChecked(checked);
		return;
	}

	if(checked){
		// m_toolButton->setArrowType(Qt::DownArrow);
		if(_widget)
			_widget->show();
	}
	else{
		// m_toolButton->setArrowType(Qt::RightArrow);
		if(_widget)
			_widget->hide();
	}
}

void ExtendibleWidget::toggle()
{
	setChecked(!_toolButton->isChecked());
}
