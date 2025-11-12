/*
 * Copyright (c) 2008-2015:  G-CSC, Goethe University Frankfurt
 * Copyright (c) 2006-2008:  Steinbeis Forschungszentrum (STZ Ölbronn)
 * Copyright (c) 2006-2015:  Sebastian Reiter
 * Author: Markus Knodel
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

#ifndef TOOL_FRAC_TO_LAYER_ARTE_H
#define TOOL_FRAC_TO_LAYER_ARTE_H

#include <QtWidgets>
#include <vector>
#include "app.hpp"
#include "standard_tools.hpp"
//#include "lib_grid/algorithms/extrusion/expand_layers.h"
#include "tool_frac_to_layer.hpp"
//#include "lib_grid/algorithms/extrusion/expand_layers_arte.h"

class FracToLayerWidgetArte : public FracToLayerWidget {
	Q_OBJECT

	public:
		FracToLayerWidgetArte(const QString& name, QWidget* parent, ITool* tool);

		~FracToLayerWidgetArte() override = default;


		bool degenerated_fractures() const override;
		bool expand_outer_boundaries() const override;

		bool diamondsUseTriangles() const;
		bool establishDiamonds() const;

		protected slots:;

		void applyClicked() override;




	protected:

		QCheckBox*	_use_triangles_in_diamons;
		QCheckBox*	_establish_diamonds;

};

#endif
