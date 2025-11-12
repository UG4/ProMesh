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

#include "tool_frac_to_layer_arte.hpp"

FracToLayerWidgetArte::
FracToLayerWidgetArte(const QString& name, QWidget* parent,
				  ITool* tool) :
				  FracToLayerWidget(parent)
{
//	UG_LOG("construct FAA" << std::endl);

	_tool = tool;
	_object = nullptr;
	QString title = name;
	title.append(": ");

//	create the layouts
	auto* vLayout = new QVBoxLayout(this);

//	UG_LOG("construct FA 1" << std::endl);


////	add a checkbox that allows to choose whether we have to create degenerated fractures
//	m_cbCreateDegenerated = new QCheckBox(this);
//	m_cbCreateDegenerated->setText(tr("degenerated layers"));
//	m_cbCreateDegenerated->setChecked(false);
//	vLayout->addWidget(m_cbCreateDegenerated);
//
//	UG_LOG("construct FA 1a" << std::endl);

//
////	add a checkbox that allows to choose whether we expand fractures at inner boundaries
//	m_cbExpandOuterBounds = new QCheckBox(this);
//	m_cbExpandOuterBounds->setText(tr("expand outer boundaries"));
//	m_cbExpandOuterBounds->setText(tr("blubb"));
//	m_cbExpandOuterBounds->setChecked(true);
//	vLayout->addWidget(m_cbExpandOuterBounds);

	//	add a checkbox that allows to choose whether the diamonds should use only triangles
	_use_triangles_in_diamons = new QCheckBox(this);

//	UG_LOG("construct FA X 1" << std::endl);

	_use_triangles_in_diamons->setText(tr("triangles in diamonds"));

//	UG_LOG("construct FA X 2" << std::endl);

	_use_triangles_in_diamons->setChecked(false);

//	UG_LOG("construct FA X 3" << std::endl);

	vLayout->addWidget(_use_triangles_in_diamons);

//	UG_LOG("construct FA 2" << std::endl);

	//	add a checkbox that allows to choose whether we want to have diamonds
	_establish_diamonds = new QCheckBox(this);
	_establish_diamonds->setText(tr("establish diamonds"));
	_establish_diamonds->setChecked(true);
	vLayout->addWidget(_establish_diamonds);
//
//	UG_LOG("construct FA 3" << std::endl);


//	create a hbox-layout for the add-button
	auto* hAddLayout = new QHBoxLayout();
	vLayout->addLayout(hAddLayout);

	auto* btnAdd = new QPushButton(tr("add subset"), this);
	connect(btnAdd, &QPushButton::clicked, this, &FracToLayerWidgetArte::addClicked);

	_q_subset_index = new QSpinBox(this);
	_q_subset_index->setRange(0, 1e+9);
	_q_subset_index->setValue(0);
	_q_subset_index->setSingleStep(1);
	hAddLayout->addWidget(btnAdd);
	hAddLayout->addWidget(_q_subset_index);

//	UG_LOG("construct FA Spin" << std::endl);


//	create a list box
	_list_widget = new QListWidget(this);
	vLayout->addWidget(_list_widget);
	connect(_list_widget, &QListWidget::currentItemChanged, this, &FracToLayerWidgetArte::currentItemChanged);

//	create the layout for the input boxes
	auto* formLayout = new QFormLayout();
	formLayout->setSpacing(5);
	formLayout->setHorizontalSpacing(10);
	formLayout->setVerticalSpacing(8);
	vLayout->addLayout(formLayout);

//	create the input boxes
	_q_width = new QDoubleSpinBox(this);
	_q_width->setValue(0.01);
	_q_width->setDecimals(9);
	_q_width->setRange(0, 1e+9);
	_q_width->setSingleStep(0.01);
	connect(_q_width, &QDoubleSpinBox::valueChanged, this, &FracToLayerWidgetArte::widthChanged);
	formLayout->addRow(tr("layer-width:"), _q_width);

	_q_new_subset = new QSpinBox(this);
	_q_new_subset->setValue(0);
	_q_new_subset->setRange(0, 1e+9);
	_q_new_subset->setSingleStep(1);
	formLayout->addRow(tr("new subset:"), _q_new_subset);
	connect(_q_new_subset, &QSpinBox::valueChanged, this, &FracToLayerWidgetArte::newSubsetIndexChanged);

//	create ok and cancel buttons
	auto* hDoneLayout = new QHBoxLayout();
	vLayout->addLayout(hDoneLayout);

	auto btnApply = new QPushButton(tr("Apply"), this);
	connect(btnApply, &QPushButton::clicked, this, &FracToLayerWidgetArte::applyClicked);
	hDoneLayout->addWidget(btnApply);

	auto* btnClear = new QPushButton(tr("Clear"), this);
	connect(btnClear, &QPushButton::clicked, this, &FracToLayerWidgetArte::clearClicked);
	hDoneLayout->addWidget(btnClear);

	hDoneLayout->addStretch();

//	UG_LOG("End construct FA" << std::endl);

}


//const FracToLayerWidget::SubsetEntryVec& FracToLayerWidget::
//entries()	const
//{return m_entries;}
//
//size_t FracToLayerWidget::
//numEntries() const
//{return m_entries.size();}
//
//const FracToLayerWidget::SubsetEntry& FracToLayerWidget::
//entry(size_t index) const
//{return m_entries.at(index);}
//
bool FracToLayerWidgetArte::
degenerated_fractures() const
{
	return false;
}

bool FracToLayerWidgetArte::
expand_outer_boundaries() const
{
	return false;
}

bool FracToLayerWidgetArte::diamondsUseTriangles() const
{
	return _use_triangles_in_diamons->isChecked();
}

bool FracToLayerWidgetArte::establishDiamonds() const
{
	return _establish_diamonds->isChecked();
}


//void FracToLayerWidget::
//addClicked()
//{
//	if(!m_object){
//		m_object = app::getActiveObject();
//		if(m_object){
//			QString title = this->windowTitle();
//			title.append(m_object->name());
//			this->setWindowTitle(title);
//		}
//		else{
//			QMessageBox msg(this);
//			msg.setText(tr("WARNING: Can't operate on invalid object.\n"
//							"Please select a valid object in the scene inspector first."));
//			msg.exec();
//			return;
//		}
//	}
//
//	if(m_object != app::getActiveObject()){
//		QMessageBox msg(this);
//		msg.setText(tr("WARNING: The active object has changed. Clear will be "
//						"performed before the subset is added."));
//		msg.exec();
//		clearClicked();
//		m_object = app::getActiveObject();
//	}
//
////	add a new entry - if it not already exists
//	int si = m_qSubsetIndex->value();
//
//	for(size_t i = 0; i < m_entries.size(); ++i){
//		if(m_entries[i].subsetIndex == si){
//			QMessageBox msg(this);
//			msg.setText(tr("WARNING: Entry already exists."));
//			msg.exec();
//			return;
//		}
//	}
//
////	make sure that the entry is valid
//	if((si < 0) || (si >= m_object->num_subsets())){
//		QMessageBox msg(this);
//		msg.setText(tr("WARNING: Invalid subset index."));
//		msg.exec();
//		return;
//	}
//
//	m_entries.push_back(SubsetEntry(si, 0, 0));
//	QString itemName = QString::number(si);
//	itemName.append(": ").append(m_object->get_subset_name(si));
//	QListWidgetItem*nItem = new QListWidgetItem(itemName, m_listWidget);
//	m_listWidget->setCurrentItem(nItem);
//}
//
void FracToLayerWidgetArte::
applyClicked()
{
	if(_object != app::getActiveObject()){
		QMessageBox msg(this);
		msg.setText(tr("Sorry - the active object is not the same as the"
				" one for which the subsets were added. Aborting."));
		msg.exec();
		return;
	}

//	if degenerated is set to true, then all widths are set to 0
//	if(degenerated_fractures()){
//		for(size_t i = 0; i < m_entries.size(); ++i){
//			m_entries[i].width = 0;
//		}
//	}

//	now run the tool
	try{
		_tool->execute(_object, this);
	}
	catch(ug::UGError error){
		UG_LOG("Execution of tool " << _tool->get_name() << " failed with the following message:\n");
		UG_LOG("  " << error.get_msg() << std::endl);
	}
}
//
//void FracToLayerWidget::
//clearClicked()
//{
//	m_entries.clear();
//	m_listWidget->clear();
//	m_object = nullptr;
//}
//
//void FracToLayerWidget::
//currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
//{
////	update input windows
////	get the current index
//	int curInd = m_listWidget->currentIndex().row();
//	if((curInd >= 0) && curInd < (int)m_entries.size()){
//		m_qWidth->setValue(m_entries[curInd].width);
//		m_qNewSubset->setValue(m_entries[curInd].newSubsetIndex);
//	}
//}
//
//void FracToLayerWidget::
//widthChanged(double width){
//	int curInd = m_listWidget->currentIndex().row();
//	if((curInd >= 0) && curInd < (int)m_entries.size()){
//		m_entries[curInd].width = width;
//	}
//}
//
//void FracToLayerWidget::
//newSubsetIndexChanged(int newInd){
//	int curInd = m_listWidget->currentIndex().row();
//	if((curInd >= 0) && curInd < (int)m_entries.size()){
//		m_entries[curInd].newSubsetIndex = newInd;
//	}
//}
