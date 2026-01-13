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

#include "tool_frac_to_layer_arte3D.h"

FracToLayerWidgetArte3D::
FracToLayerWidgetArte3D(const QString& name, QWidget* parent, ITool* tool)
	: FracToLayerWidget(parent)
{

	m_tool = tool;
	m_object = nullptr;
	QString title = name;
	title.append(": ");

	QVBoxLayout* vLayout = new QVBoxLayout(this);

	//	add a checkbox that allows to choose whether the diamonds should be in preform only or complete
	m_diamonsOnlyPreform = new QCheckBox(this);

	m_diamonsOnlyPreform->setText(tr("only preform of diamonds"));

	m_diamonsOnlyPreform->setChecked(false);

	vLayout->addWidget(m_diamonsOnlyPreform);

	//	add a checkbox that allows to choose whether we want to have diamonds
	m_establishDiamonds = new QCheckBox(this);
	m_establishDiamonds->setText(tr("establish diamonds"));
	m_establishDiamonds->setChecked(true);
	vLayout->addWidget(m_establishDiamonds);

//	create a hbox-layout for the add-button
	QHBoxLayout* hAddLayout = new QHBoxLayout();
	vLayout->addLayout(hAddLayout);

	QPushButton* btnAdd = new QPushButton(tr("add subset"), this);
	connect(btnAdd, SIGNAL(clicked()), this, SLOT(addClicked()));

	m_qSubsetIndex = new QSpinBox(this);
	m_qSubsetIndex->setRange(0, 1e+9);
	m_qSubsetIndex->setValue(0);
	m_qSubsetIndex->setSingleStep(1);
	hAddLayout->addWidget(btnAdd);
	hAddLayout->addWidget(m_qSubsetIndex);

//	create a list box
	m_listWidget = new QListWidget(this);
	vLayout->addWidget(m_listWidget);
	connect(m_listWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
			this, SLOT(currentItemChanged(QListWidgetItem*,QListWidgetItem*)));

//	create the layout for the input boxes
	QFormLayout* formLayout = new QFormLayout();
	formLayout->setSpacing(5);
	formLayout->setHorizontalSpacing(10);
	formLayout->setVerticalSpacing(8);
	vLayout->addLayout(formLayout);

//	create the input boxes
	m_qWidth = new QDoubleSpinBox(this);
	m_qWidth->setValue(0.01);
	m_qWidth->setDecimals(9);
	m_qWidth->setRange(0, 1e+9);
	m_qWidth->setSingleStep(0.01);
	connect(m_qWidth, SIGNAL(valueChanged(double)), this, SLOT(widthChanged(double)));
	formLayout->addRow(tr("layer-width:"), m_qWidth);

	m_qNewSubset = new QSpinBox(this);
	m_qNewSubset->setValue(0);
	m_qNewSubset->setRange(0, 1e+9);
	m_qNewSubset->setSingleStep(1);
	formLayout->addRow(tr("new subset:"), m_qNewSubset);
	connect(m_qNewSubset, SIGNAL(valueChanged(int)), this, SLOT(newSubsetIndexChanged(int)));

//	create ok and cancel buttons
	QHBoxLayout* hDoneLayout = new QHBoxLayout();
	vLayout->addLayout(hDoneLayout);

	QPushButton* btnApply = new QPushButton(tr("Apply"), this);
	connect(btnApply, SIGNAL(clicked()), this, SLOT(applyClicked()));
	hDoneLayout->addWidget(btnApply);

	QPushButton* btnClear = new QPushButton(tr("Clear"), this);
	connect(btnClear, SIGNAL(clicked()), this, SLOT(clearClicked()));
	hDoneLayout->addWidget(btnClear);

	hDoneLayout->addStretch();

//	UG_LOG("End construct FA" << std::endl);

}

FracToLayerWidgetArte3D::
~FracToLayerWidgetArte3D()	{}

bool FracToLayerWidgetArte3D::
degenerated_fractures() const
{
	return false;
}

bool FracToLayerWidgetArte3D::
expand_outer_boundaries() const
{
	return false;
}

bool FracToLayerWidgetArte3D::diamondsOnlyPreform() const
{
	return m_diamonsOnlyPreform->isChecked();
}

bool FracToLayerWidgetArte3D::establishDiamonds() const
{
	return m_establishDiamonds->isChecked();
}



void FracToLayerWidgetArte3D::
applyClicked()
{
	if(m_object != app::getActiveObject()){
		QMessageBox msg(this);
		msg.setText(tr("Sorry - the active object is not the same as the"
				" one for which the subsets were added. Aborting."));
		msg.exec();
		return;
	}

//	now run the tool
	try{
		m_tool->execute(m_object, this);
	}
	catch(ug::UGError error){
		UG_LOG("Execution of tool " << m_tool->get_name() << " failed with the following message:\n");
		UG_LOG("  " << error.get_msg() << std::endl);
	}
}
