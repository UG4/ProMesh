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

#include "truncated_double_spin_box.h"

TruncatedDoubleSpinBox::
TruncatedDoubleSpinBox(QWidget* parent) :
	QDoubleSpinBox(parent)
{
	connect (this,
			 static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			 this,
			 static_cast<void(TruncatedDoubleSpinBox::*)(double)>(&TruncatedDoubleSpinBox::valueChanged));

	// connect (static_cast<QDoubleSpinBox*>(this),
	// 		 SIGNAL(static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged)),
	// 		 this, SIGNAL(valueChanged (double)));
}

QString TruncatedDoubleSpinBox::
textFromValue(double value) const
{
	QString s = QDoubleSpinBox::textFromValue(value);
//	check if a decimal point is contained. If this is the
//	case, remove all zeros at the end of the string
	//chop, truncate, resize left

	int pointInd = s.indexOf('.');
	if(pointInd == -1)
		return s;

	int len = s.length();
	while((len > 0) && (s[len - 1] == '0'))
		--len;

	if(pointInd == len - 1)
		--len;
	
	return s.left(len);
}
