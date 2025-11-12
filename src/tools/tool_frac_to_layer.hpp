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

#ifndef TOOL_FRAC_TO_LAYER_H
#define TOOL_FRAC_TO_LAYER_H

#include <QtWidgets>
#include <vector>
#include "app.hpp"
#include "standard_tools.hpp"
#include "lib_grid/algorithms/extrusion/expand_layers.h"
//#include "lib_grid/algorithms/extrusion/expand_layers_arte.h"

class FracToLayerWidget : public QWidget {
	Q_OBJECT

	public:
		using SubsetEntry = ug::FractureInfo;
		using SubsetEntryVec = std::vector<SubsetEntry>;

	public:
		FracToLayerWidget(const QString& name, QWidget* parent, ITool* tool);

		~FracToLayerWidget() override = default;

		const SubsetEntryVec& entries()	const;
		size_t numEntries() const;
		const SubsetEntry& entry(size_t index) const;

		virtual bool degenerated_fractures() const;
		virtual bool expand_outer_boundaries() const;

	protected slots:;
		void addClicked();

	virtual void applyClicked();

		void clearClicked();

		void currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);

		void widthChanged(double width);

		void newSubsetIndexChanged(int newInd);

	protected:
		LGObject*		_object;
		ITool*			_tool;
		QListWidget*	_list_widget;
		QSpinBox*		_q_subset_index;
		QDoubleSpinBox* _q_width;
		QCheckBox*		_cb_create_degenerated;
		QCheckBox*		_cb_expand_outer_bounds;
		QSpinBox*		_q_new_subset;
		SubsetEntryVec	_entries;

		FracToLayerWidget(QWidget* parent ) : QWidget(parent) {};

};

#endif