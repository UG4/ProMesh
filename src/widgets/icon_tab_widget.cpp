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
 
#include "icon_tab_widget.hpp"

#include <iostream>
#include <QVBoxLayout>
#include <QToolBar>
#include <QToolButton>
#include <QStackedWidget>


IconTabWidget::IconTabWidget(QWidget* parent) : QWidget(parent)
{
	_tool_bar = new QToolBar(this);
	_tool_bar->setIconSize(QSize(24, 24));

	_stacked_widget = new QStackedWidget(this);

	auto* layout = new QVBoxLayout();
	layout->addWidget(_tool_bar);
	layout->addWidget(_stacked_widget);
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);
	this->setLayout(layout);

}

IconTabWidget::~IconTabWidget()
{

}

void IconTabWidget::ic_button_click() {
	std::cout << "IconTabWidget::ic_button_click" << std::endl;
}

void IconTabWidget::addPage(QWidget* page, const QIcon& icon, const QString& tooltip)
{
	auto* toolBtn = new QToolButton(_tool_bar);
	toolBtn->setIcon(icon);
	toolBtn->setCheckable(true);
	toolBtn->setAutoExclusive(true);
	toolBtn->setToolTip(tooltip);


	int index = _stacked_widget->count();
	connect(toolBtn, &QToolButton::clicked, this, [this, index]() {
		_stacked_widget->setCurrentIndex(index);
	});

	_tool_bar->addWidget(toolBtn);
	_stacked_widget->addWidget(page);

//	if this is the first page, we'll select it
	if(_stacked_widget->count() == 1){
		toolBtn->setChecked(true);
		_stacked_widget->setCurrentIndex(0);
	}
}

int IconTabWidget::count()
{
	return _stacked_widget->count();
}

QWidget* IconTabWidget::widget(int pageIndex)
{
	return _stacked_widget->widget(pageIndex);
}
