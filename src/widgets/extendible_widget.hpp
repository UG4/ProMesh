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

#ifndef __H__UG__extendible_widget__
#define __H__UG__extendible_widget__

#include <QWidget>
#include <QLabel>

class QToolButton;
class QVBoxLayout;
class ExtendibleWidgetHeader;


class ExtendibleWidget : public QFrame {
	Q_OBJECT

	public:
		explicit ExtendibleWidget(QWidget* parent);
		~ExtendibleWidget() override = default;

		void setText(const QString& text);
		void setWidget(QWidget* widget);
		void setInfoText(const QString& text);

		QWidget* widget()	{return _widget;}
		
	public slots:
		void setChecked(bool checked);
		void toggle();

	private:
		QVBoxLayout*			_v_layout;
		QToolButton*			_toolButton;
		QLabel*					_header;
		QWidget*				_widget;
};



class ExtendibleWidgetHeader : public QLabel {
	Q_OBJECT

	public:
		explicit ExtendibleWidgetHeader(QWidget* parent) : QLabel(parent)	{}
		~ExtendibleWidgetHeader() override = default;

	signals:
		void clicked();
		void double_clicked();

	protected:
		void mouseReleaseEvent(QMouseEvent* evt) override {
			QLabel::mouseReleaseEvent(evt);
			emit(clicked());
		}

		void mouseDoubleClickEvent(QMouseEvent* evt) override {
			QLabel::mouseDoubleClickEvent(evt);
			emit(double_clicked());
		}
};


#endif
