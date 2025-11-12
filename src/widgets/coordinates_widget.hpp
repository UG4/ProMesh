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

#ifndef __H__COORDINATES_WIDGET
#define __H__COORDINATES_WIDGET

#include <QFrame>
#include <QString>
#include <sstream>
#include <iomanip>
#include "app.hpp"
#include "../tools/standard_tools.hpp"
#include "common/error.h"


class TruncatedDoubleSpinBox;
class QLineEdit;

class CoordinatesWidget: public QFrame {
	Q_OBJECT;

	public:
		CoordinatesWidget(const QString& name, QWidget* parent,
						  ITool* tool, bool apply_on_change = false,
						  bool show_apply_button = true);

		void set_coords(double x, double y, double z);

		[[nodiscard]] double x() const;
		[[nodiscard]] double y() const;
		[[nodiscard]] double z() const;

	protected slots:
		void valueChanged(double);

		void textEdited(const QString& new_text);
		
		void apply();
		
	protected:
		ITool* _tool;
		TruncatedDoubleSpinBox*	_x;
		TruncatedDoubleSpinBox*	_y;
		TruncatedDoubleSpinBox*	_z;
		QLineEdit* _line_edit;
		bool _apply_on_change;
		bool _refreshing_coords;
};

#endif