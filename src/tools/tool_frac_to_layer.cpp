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

#include "tool_frac_to_layer.hpp"

FracToLayerWidget::
FracToLayerWidget(const QString& name, QWidget* parent,
				  ITool* tool) :
	QWidget(parent)
{
	_tool = tool;
	_object = nullptr;
	QString title = name;
	title.append(": ");

//	create the layouts
	auto* vLayout = new QVBoxLayout(this);


//	add a checkbox that allows to choose whether we have to create degenerated fractures
	_cb_create_degenerated = new QCheckBox(this);
	_cb_create_degenerated->setText(tr("degenerated layers"));
	_cb_create_degenerated->setChecked(false);
	vLayout->addWidget(_cb_create_degenerated);

//	add a checkbox that allows to choose whether we expand fractures at inner boundaries
	_cb_expand_outer_bounds = new QCheckBox(this);
	_cb_expand_outer_bounds->setText(tr("expand outer boundaries"));
	_cb_expand_outer_bounds->setChecked(true);
	vLayout->addWidget(_cb_expand_outer_bounds);

//	create a hbox-layout for the add-button
	auto* hAddLayout = new QHBoxLayout();
	vLayout->addLayout(hAddLayout);

	auto* btnAdd = new QPushButton(tr("add subset"), this);
	connect(btnAdd, &QPushButton::clicked, this, &FracToLayerWidget::addClicked);

	_q_subset_index = new QSpinBox(this);
	_q_subset_index->setRange(0, 1e+9);
	_q_subset_index->setValue(0);
	_q_subset_index->setSingleStep(1);
	hAddLayout->addWidget(btnAdd);
	hAddLayout->addWidget(_q_subset_index);

//	create a list box
	_list_widget = new QListWidget(this);
	vLayout->addWidget(_list_widget);
	connect(_list_widget, &QListWidget::currentItemChanged, this, &FracToLayerWidget::currentItemChanged);

//	create the layout for the input boxes
	QFormLayout* formLayout = new QFormLayout();
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
	connect(_q_width, &QDoubleSpinBox::valueChanged, this, &FracToLayerWidget::widthChanged);
	formLayout->addRow(tr("layer-width:"), _q_width);

	_q_new_subset = new QSpinBox(this);
	_q_new_subset->setValue(0);
	_q_new_subset->setRange(0, 1e+9);
	_q_new_subset->setSingleStep(1);
	formLayout->addRow(tr("new subset:"), _q_new_subset);
	connect(_q_new_subset, &QSpinBox::valueChanged, this, &FracToLayerWidget::newSubsetIndexChanged);

//	create ok and cancel buttons
	auto* hDoneLayout = new QHBoxLayout();
	vLayout->addLayout(hDoneLayout);

	auto* btnApply = new QPushButton(tr("Apply"), this);
	connect(btnApply, &QPushButton::clicked, this, &FracToLayerWidget::applyClicked);
	hDoneLayout->addWidget(btnApply);

	auto* btnClear = new QPushButton(tr("Clear"), this);
	connect(btnClear, &QPushButton::clicked, this, &FracToLayerWidget::clearClicked);
	hDoneLayout->addWidget(btnClear);

	hDoneLayout->addStretch();
}



const FracToLayerWidget::SubsetEntryVec& FracToLayerWidget::
entries()	const
{return _entries;}

size_t FracToLayerWidget::
numEntries() const
{return _entries.size();}

const FracToLayerWidget::SubsetEntry& FracToLayerWidget::
entry(size_t index) const
{return _entries.at(index);}

bool FracToLayerWidget::
degenerated_fractures() const
{return _cb_create_degenerated->isChecked();}

bool FracToLayerWidget::
expand_outer_boundaries() const
{return _cb_expand_outer_bounds->isChecked();}

void FracToLayerWidget::
addClicked()
{
	if(!_object){
		_object = app::getActiveObject();
		if(_object){
			QString title = this->windowTitle();
			title.append(_object->name());
			this->setWindowTitle(title);
		}
		else{
			QMessageBox msg(this);
			msg.setText(tr("WARNING: Can't operate on invalid object.\n"
							"Please select a valid object in the scene inspector first."));
			msg.exec();
			return;
		}
	}

	if(_object != app::getActiveObject()){
		QMessageBox msg(this);
		msg.setText(tr("WARNING: The active object has changed. Clear will be "
						"performed before the subset is added."));
		msg.exec();
		clearClicked();
		_object = app::getActiveObject();
	}

//	add a new entry - if it not already exists
	int si = _q_subset_index->value();

	for(size_t i = 0; i < _entries.size(); ++i){
		if(_entries[i].subsetIndex == si){
			QMessageBox msg(this);
			msg.setText(tr("WARNING: Entry already exists."));
			msg.exec();
			return;
		}
	}

//	make sure that the entry is valid
	if((si < 0) || (si >= _object->num_subsets())){
		QMessageBox msg(this);
		msg.setText(tr("WARNING: Invalid subset index."));
		msg.exec();
		return;
	}

	_entries.emplace_back(si, 0, 0);
	QString itemName = QString::number(si);
	itemName.append(": ").append(_object->get_subset_name(si));
	auto*nItem = new QListWidgetItem(itemName, _list_widget);
	_list_widget->setCurrentItem(nItem);
}

void FracToLayerWidget::
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
	if(degenerated_fractures()){
		for(size_t i = 0; i < _entries.size(); ++i){
			_entries[i].width = 0;
		}
	}

//	now run the tool
	try{
		_tool->execute(_object, this);
	}
	catch(ug::UGError error){
		UG_LOG("Execution of tool " << _tool->get_name() << " failed with the following message:\n");
		UG_LOG("  " << error.get_msg() << std::endl);
	}
}

void FracToLayerWidget::
clearClicked()
{
	_entries.clear();
	_list_widget->clear();
	_object = nullptr;
}

void FracToLayerWidget::
currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
//	update input windows
//	get the current index
	int curInd = _list_widget->currentIndex().row();
	if((curInd >= 0) && curInd < (int)_entries.size()){
		_q_width->setValue(_entries[curInd].width);
		_q_new_subset->setValue(_entries[curInd].newSubsetIndex);
	}
}

void FracToLayerWidget::
widthChanged(double width){
	int curInd = _list_widget->currentIndex().row();
	if((curInd >= 0) && curInd < (int)_entries.size()){
		_entries[curInd].width = width;
	}
}

void FracToLayerWidget::
newSubsetIndexChanged(int newInd){
	int curInd = _list_widget->currentIndex().row();
	if((curInd >= 0) && curInd < (int)_entries.size()){
		_entries[curInd].newSubsetIndex = newInd;
	}
}
