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
#include <QScrollArea>
#include <QPushButton>
#include <QResizeEvent>
#include <QFrame>
#include "widget_container.hpp"
#include "widget_list.hpp"


WidgetList::WidgetList(QWidget* parent) : QWidget(parent)
{
	_scroll_area = new QScrollArea(this);
	//m_scrollArea->setBackgroundRole(QPalette::Dark);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(_scroll_area);
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);
	this->setLayout(layout);

//	This widget will contain the widgetContainer and a spacer
	auto* spacerWidget = new QWidget(_scroll_area);
	auto* spacerLayout = new QVBoxLayout(spacerWidget);
	spacerLayout->setSpacing(0);
	spacerLayout->setContentsMargins(0, 0, 0, 0);
	spacerWidget->setLayout(spacerLayout);

	_widget_container = new WidgetContainer(spacerWidget);
	spacerLayout->addWidget(_widget_container);

	spacerLayout->addStretch();

	_scroll_area->setWidget(spacerWidget);
	_scroll_area->setWidgetResizable(true);
}


void WidgetList::addWidget(QWidget* widget, Qt::Alignment alignment)
{
	_widget_container->addWidget(widget, alignment);
}

