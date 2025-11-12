/*
 * Copyright (c) 2016:  G-CSC, Goethe University Frankfurt
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

#include <QScrollArea>
#include "property_widget.hpp"

PropertyWidget::
PropertyWidget(QWidget* parent) :
	QFrame(parent),
	_content(nullptr)
{
	_scroll_area = new QScrollArea(this);
	//m_scrollArea->setBackgroundRole(QPalette::Dark);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(_scroll_area);
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);
	this->setLayout(layout);

//	This widget will contain the widgetContainer and a spacer
	_spacer_widget = new QWidget(_scroll_area);
	_spacer_layout = new QVBoxLayout(_spacer_widget);
	_spacer_layout->setSpacing(0);
	_spacer_layout->setContentsMargins(0, 0, 0, 0);
	_spacer_widget->setLayout(_spacer_layout);

	_scroll_area->setWidget(_spacer_widget);
	_scroll_area->setWidgetResizable(true);
}
