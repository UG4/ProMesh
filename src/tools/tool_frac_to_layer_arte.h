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
#include "app.h"
#include "standard_tools.h"
//#include "lib_grid/algorithms/extrusion/expand_layers.h"
#include "tool_frac_to_layer.h"
//#include "lib_grid/algorithms/extrusion/expand_layers_arte.h"

class FracToLayerWidgetArte : public FracToLayerWidget
{
	Q_OBJECT

//	public:
//		typedef ug::FractureInfo SubsetEntry;
//		typedef std::vector<SubsetEntry>	SubsetEntryVec;

	public:
		FracToLayerWidgetArte(const QString& name, QWidget* parent, ITool* tool);
		virtual ~FracToLayerWidgetArte();

//		const SubsetEntryVec& entries()	const;
//		size_t numEntries() const;
//		const SubsetEntry& entry(size_t index) const;

		bool degenerated_fractures() const;
		bool expand_outer_boundaries() const;

		bool diamondsUseTriangles() const;
		bool establishDiamonds() const;

		protected slots:;
//			void addClicked();

			void applyClicked();


//	protected slots:;
//		void addClicked();
//
//		void applyClicked();
//
//		void clearClicked();
//
//		void currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
//
//		void widthChanged(double width);
//
//		void newSubsetIndexChanged(int newInd);
//
//	protected:
//		LGObject*		m_object;
//		ITool*			m_tool;
//		QListWidget*	m_listWidget;
//		QSpinBox*		m_qSubsetIndex;
//		QDoubleSpinBox* m_qWidth;
//		QCheckBox*		m_cbCreateDegenerated;
//		QCheckBox*		m_cbExpandOuterBounds;
//		QSpinBox*		m_qNewSubset;
//		SubsetEntryVec	m_entries;

	protected:

		QCheckBox*	m_useTrianglesInDiamons;
		QCheckBox*	m_establishDiamonds;

};

#endif // TOOL_FRAC_TO_LAYER_ARTE_H
