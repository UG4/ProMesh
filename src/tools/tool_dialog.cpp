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

#include <QtWidgets>
#include <vector>
#include "common/log.h"
#include "tool_dialog.hpp"
#include "tool_manager.hpp"
#include "app.hpp"
#include "widgets/matrix_widget.hpp"
#include "../widgets/double_slider.hpp"
#include "../widgets/truncated_double_spin_box.hpp"

using namespace std;



ToolWidget::ToolWidget(const QString& name, QWidget* parent, ITool* tool, uint buttons) :
	QFrame(parent),
	_current_form_layout(nullptr) {
	_tool = tool;

	setFrameStyle(StyledPanel | Sunken);

	auto baseLayout = new QVBoxLayout(this);
	baseLayout->setSpacing(2);

	auto* vBoxLayout = new QVBoxLayout();
	vBoxLayout->setSpacing(2);

	_main_layout = vBoxLayout;
	baseLayout->addLayout(vBoxLayout);

	//æ connect(m_signalMapper, SIGNAL(mapped(int)), this, &ToolWidget::buttonClicked); // args: int


	if(buttons & IDB_APPLY){
		auto btn = new QPushButton(tr("Apply"), this);
		baseLayout->addWidget(btn, 0, Qt::AlignLeft);

		connect(btn, &QPushButton::clicked, this, [this]() {
			buttonClicked(IDB_APPLY);
		});

		//connect(m_valueSignalMapper, SIGNAL(mapped(int)),
		//		this, SIGNAL(valueChanged(int)));

	}
}

QFormLayout* ToolWidget::current_form_layout()
{
	if(!_current_form_layout){
		_current_form_layout = new QFormLayout();
		_current_form_layout->setSpacing(2);
		_current_form_layout->setHorizontalSpacing(2);
		_current_form_layout->setVerticalSpacing(8);
		_main_layout->addLayout(_current_form_layout);
	}
	return _current_form_layout;
}

void ToolWidget::addWidget(const QString& caption, QWidget* widget)
{
	current_form_layout()->addRow(caption, widget);
	_widgets.emplace_back(widget, WT_WIDGET);
}

void ToolWidget::addSlider(const QString& caption,
							double min, double max, double value)
{
	auto* slider = new DoubleSlider(this);
	slider->setRange(min, max);
	slider->setValue(value);
	current_form_layout()->addRow(caption, slider);
	size_t index = _widgets.size();
	connect(slider, &DoubleSlider::valueChanged, this, [this, index]() { valueChanged((int)index); }); //ø  // arg: none
	_widgets.emplace_back(slider, WT_SLIDER);
}

void ToolWidget::addSpinBox(const QString& caption,
							double min, double max, double value,
							double stepSize, int numDecimals)
{
	auto* spinner = new TruncatedDoubleSpinBox(this);
	spinner->setLocale(QLocale(tr("C")));
	spinner->setRange(min, max);
	spinner->setDecimals(numDecimals);
	spinner->setSingleStep(stepSize);
	spinner->setValue(value);
	current_form_layout()->addRow(caption, spinner);

	size_t index = (int)_widgets.size();
	connect(spinner, &TruncatedDoubleSpinBox::valueChanged, this, [this, index]() { valueChanged((int)index); }); // arg double
	

	_widgets.emplace_back(spinner, WT_SPIN_BOX);
}

void ToolWidget::addComboBox(const QString& caption,
							const QStringList& entries,
							int activeEntry)
{
	auto* combo = new QComboBox(this);
	combo->addItems(entries);
	combo->setCurrentIndex(activeEntry);
	current_form_layout()->addRow(caption, combo);
	size_t index = _widgets.size();
	connect(combo, &QComboBox::currentIndexChanged, this, [this, index]() { valueChanged((int)index); });  //ø // arg: int
	_widgets.emplace_back(combo, WT_COMBO_BOX);
}

void ToolWidget::addCheckBox(const QString& caption,
							bool bChecked)
{
	auto* check = new QCheckBox(caption, this);
	check->setChecked(bChecked);
	_current_form_layout = nullptr;
	_main_layout->addWidget(check);
	size_t index = _widgets.size();
	connect(check, &QCheckBox::stateChanged, this, [this, index]() { valueChanged((int)index); });  //ø // arg: int
	_widgets.emplace_back(check, WT_CHECK_BOX);
}

void ToolWidget::addListBox(const QString& caption,
							QStringList& entries,
							bool multiSelection)
{
	auto list = new QListWidget(this);
	if(multiSelection)
		list->setSelectionMode(QAbstractItemView::MultiSelection);
	list->addItems(entries);
	current_form_layout()->addRow(caption, list);
	_widgets.emplace_back(list, WT_LIST_BOX);
}

void ToolWidget::addTextBox(const QString& caption, const QString& text)
{
	auto* textBox = new QLineEdit(this);
	textBox->setText(text);
	current_form_layout()->addRow(caption, textBox);

	size_t index = _widgets.size();
	connect(textBox, &QLineEdit::textChanged, this, [this, index]() { valueChanged((int)index); }); //ø // arg: const QString&
	_widgets.emplace_back(textBox, WT_TEXT_BOX);
}

void ToolWidget::addVector(const QString& caption, int size, double* values)
{
	const char* coordLabels[] = {"x", "y", "z", "w"};
	const char** labels = size <= 4 ? coordLabels : nullptr;
	auto* mat = new MatrixWidget(size, 1, this, labels);

	if(values){
		for(int i = 0; i < size; ++i)
			mat->set_value(i, 0, values[i]);
	}
	else{
		for(int i = 0; i < size; ++i)
			mat->set_value(i, 0, 0);
	}

	mat->setContentsMargins(10, 0, 0, 0);

	current_form_layout()->addRow(new QLabel(caption, this));
	current_form_layout()->addRow(mat);
	size_t index = _widgets.size();
	connect(mat, static_cast<void (MatrixWidget::*)()>(&MatrixWidget::valueChanged), this,[this, index]() { valueChanged((int)index); }); //ø  // arg:none
	_widgets.emplace_back(mat, WT_MATRIX);
}

void ToolWidget::addMatrix(const QString& caption, int numRows, int numCols)
{
	auto* mat = new MatrixWidget(numRows, numCols, this);
	current_form_layout()->addRow(caption, mat);

	size_t index = _widgets.size();
	connect(mat, static_cast<void (MatrixWidget::*)()>(&MatrixWidget::valueChanged), this, [this, index]() { valueChanged((int)index); }); //ø  // args: none
	_widgets.emplace_back(mat, WT_MATRIX);
}

void ToolWidget::addFileBrowser(const QString& caption, FileWidgetType fwt,
								const QString& filter)
{
	auto* fw = new FileWidget(fwt, filter, this);
	current_form_layout()->addRow(caption, fw);
	_widgets.emplace_back(fw, WT_FILE_BROWSER);
}

void ToolWidget::buttonClicked(int buttonID)
{
	LGObject* obj = app::getActiveObject();
	if(!obj){
	//todo: create the appropriate object for the current module
		obj = app::createEmptyObject("new mesh", SOT_LG);
	}
	
	if(_tool && (obj || _tool->accepts_null_object_ptr())){
		switch(buttonID){
		case IDB_OK:
		case IDB_APPLY:
			try{
				_tool->execute(obj, this);
			}
			catch(ug::UGError error){
				UG_LOG("Execution of tool " << _tool->get_name() << " failed with the following message:\n");
				UG_LOG("  " << error.get_msg() << std::endl);
			}
			break;
		}
	}
}

void ToolWidget::clearLayout(QLayout* layout)
{
    while(layout->count() > 0){
    	QLayoutItem *item = layout->takeAt(0);
        if (item->layout()) {
            clearLayout(item->layout());
            //delete item->layout();
        }
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
}

void ToolWidget::clear()
{
	clearLayout(_main_layout);
	_current_form_layout = nullptr;
	_widgets.clear();
}

template <typename TNumber>
TNumber ToolWidget::to_number(int paramIndex, bool* bOKOut)
{
	if(bOKOut)
		*bOKOut = true;

	if(paramIndex < 0 || paramIndex >= (int)_widgets.size()){
		UG_LOG("ERROR: bad parameter index in ToolDialog::to_number: " << paramIndex << std::endl);
		if(bOKOut)
			*bOKOut = false;
		return 0;
	}

	WidgetEntry& we = _widgets[paramIndex];

	switch(we._widget_type){
	case WT_SLIDER:{
			auto* slider = dynamic_cast<DoubleSlider*>(we._widget);
			return (TNumber)slider->value();
		}break;
	case WT_SPIN_BOX:{
			auto* spinBox = qobject_cast<TruncatedDoubleSpinBox*>(we._widget);
			return (TNumber)spinBox->value();
		}break;
	case WT_COMBO_BOX:{
			auto combo = qobject_cast<QComboBox*>(we._widget);
			return (TNumber)combo->currentIndex();
		}break;
	case WT_CHECK_BOX:{
			auto* check = qobject_cast<QCheckBox*>(we._widget);
			if(check->isChecked())
				return TNumber(1);
			return TNumber(0);
		}break;
	default:
		UG_LOG("ERROR in ToolDialog::to_number: Parameter " << paramIndex << " can't be converted to a number.\n");
		if(bOKOut)
			*bOKOut = false;
		return TNumber(0);
	}
}

bool ToolWidget::to_bool(int paramIndex, bool* bOKOut)
{
	return to_number<int>(paramIndex, bOKOut) != 0;
}

int ToolWidget::to_int(int paramIndex, bool* bOKOut)
{
	return to_number<int>(paramIndex, bOKOut);
}

double ToolWidget::to_double(int paramIndex, bool* bOKOut)
{
	return to_number<double>(paramIndex, bOKOut);
}

vector<int> ToolWidget::to_index_list(int paramIndex, bool* bOKOut)
{
	if(bOKOut)
		*bOKOut = true;

//	iterate over all entries in the list. if an entry is selected, push
//	then push the associated index into the index-array.
	vector<int> outVec;

	WidgetEntry& we = _widgets[paramIndex];

	if(we._widget_type == WT_LIST_BOX){
		auto* list = qobject_cast<QListWidget*>(we._widget);
		for(int i = 0; i < list->count(); ++i){
			QListWidgetItem* item = list->item(i);
			if(item->isSelected())
				outVec.push_back(i);
		}
	}
	else{
		UG_LOG("ERROR in ToolDialog::to_number: Parameter " << paramIndex << " can't be converted to a number.\n");
		if(bOKOut)
			*bOKOut = false;
	}
	return outVec;
}

QString ToolWidget::to_string(int paramIndex, bool* bOKOut)
{
	if(bOKOut)
		*bOKOut = true;

	WidgetEntry& we = _widgets[paramIndex];

	if(we._widget_type == WT_TEXT_BOX){
		auto* textBox = qobject_cast<QLineEdit*>(we._widget);
		return textBox->text();
	}
	else if(we._widget_type == WT_FILE_BROWSER){
		auto* fw = qobject_cast<FileWidget*>(we._widget);
		return fw->filename();
	}
	else{
		UG_LOG("ERROR in ToolDialog::to_string: Parameter " << paramIndex << " can't be converted to a string-list.\n");
		if(bOKOut)
			*bOKOut = false;
	}

	return QString();
}

QStringList ToolWidget::to_string_list(int paramIndex, bool* bOKOut)
{
	if(bOKOut)
		*bOKOut = true;

	WidgetEntry& we = _widgets[paramIndex];

	if(we._widget_type == WT_FILE_BROWSER){
		auto* fw = qobject_cast<FileWidget*>(we._widget);
		return fw->filenames();
	}
	else{
		UG_LOG("ERROR in ToolDialog::to_string_list: Parameter " << paramIndex << " can't be converted to a string-list.\n");
		if(bOKOut)
			*bOKOut = false;
	}

	return QStringList();
}

ug::vector3 ToolWidget::to_vector3(int paramIndex, bool* bOKOut)
{
	if(bOKOut)
		*bOKOut = true;

	WidgetEntry& we = _widgets[paramIndex];

	if(we._widget_type == WT_MATRIX){
		auto* matWidget = qobject_cast<MatrixWidget*>(we._widget);
		ug::vector3 vec;

		for(int i = 0; i < 3; ++i)
			vec[i] = matWidget->value(i, 0);

		return vec;
	}
	else{
		UG_LOG("ERROR in ToolDialog::to_matrix33: Parameter " << paramIndex << " can't be converted to a matrix33.\n");
		if(bOKOut)
			*bOKOut = false;
	}

	return ug::vector3();
}

ug::matrix33 ToolWidget::to_matrix33(int paramIndex, bool* bOKOut)
{
	if(bOKOut)
		*bOKOut = true;

	WidgetEntry& we = _widgets[paramIndex];

	if(we._widget_type == WT_MATRIX){
		auto* matWidget = qobject_cast<MatrixWidget*>(we._widget);
		ug::matrix33 mat;

		for(int j = 0; j < 3; ++j){
			for(int i = 0; i < 3; ++i){
				mat(i, j) = matWidget->value(i, j);
			}
		}

		return mat;
	}
	else{
		UG_LOG("ERROR in ToolDialog::to_matrix33: Parameter " << paramIndex << " can't be converted to a matrix33.\n");
		if(bOKOut)
			*bOKOut = false;
	}

	return ug::matrix33();
}

ug::matrix44 ToolWidget::to_matrix44(int paramIndex, bool* bOKOut)
{
	if(bOKOut)
		*bOKOut = true;

	WidgetEntry& we = _widgets[paramIndex];

	if(we._widget_type == WT_MATRIX){
		auto* matWidget = qobject_cast<MatrixWidget*>(we._widget);
		ug::matrix44 mat;

		for(int j = 0; j < 4; ++j){
			for(int i = 0; i < 4; ++i){
				mat(i, j) = matWidget->value(i, j);
			}
		}

		return mat;
	}
	else{
		UG_LOG("ERROR in ToolDialog::to_matrix44: Parameter " << paramIndex << " can't be converted to a matrix44.\n");
		if(bOKOut)
			*bOKOut = false;
	}

	return ug::matrix44();
}

QWidget* ToolWidget::to_widget(int paramIndex, bool* bOkOut)
{
	if(paramIndex < 0 || paramIndex >= (int)_widgets.size()){
		UG_LOG("ERROR: bad parameter index in ToolDialog::to_widget: " << paramIndex << std::endl);
		if(bOkOut)
			*bOkOut = false;
		return nullptr;
	}

	WidgetEntry& we = _widgets[paramIndex];
	if(we._widget_type == WT_WIDGET){
		if(bOkOut)
			*bOkOut = true;
		return we._widget;
	}
	if(bOkOut)
		*bOkOut = false;
	return nullptr;
}

bool ToolWidget::setNumber(int paramIndex, double val)
{
	if(paramIndex < 0 || paramIndex >= (int)_widgets.size()){
		UG_LOG("ERROR: bad parameter index in ToolDialog::setNumber: " << paramIndex << std::endl);
		return false;
	}

	WidgetEntry& we = _widgets[paramIndex];

	switch(we._widget_type){
	case WT_SLIDER:{
			auto* slider = qobject_cast<QSlider*>(we._widget);
			slider->setValue(val);
		}break;
	case WT_SPIN_BOX:{
			auto* spinBox = qobject_cast<TruncatedDoubleSpinBox*>(we._widget);
			spinBox->setValue(val);
		}break;
	default:
		UG_LOG("ERROR in ToolDialog::setNumber: No matching widget found for parameter " << paramIndex << ".\n");
		return false;
	}

	return true;
}

bool ToolWidget::setString(int paramIndex, const QString& param)
{
	WidgetEntry& we = _widgets[paramIndex];

	if(we._widget_type == WT_TEXT_BOX){
		auto* textBox = qobject_cast<QLineEdit*>(we._widget);
		textBox->setText(param);
	}
	else{
		UG_LOG("ERROR in ToolDialog::set_string: Parameter " << paramIndex << " can't be converted to a string-list.\n");
		return false;
	}

	return true;
}

bool ToolWidget::setStringList(int paramIndex, const QStringList& stringList)
{
	WidgetEntry& we = _widgets[paramIndex];

	if(we._widget_type == WT_LIST_BOX){
		auto* listBox = qobject_cast<QListWidget*>(we._widget);
		listBox->clear();
		listBox->addItems(stringList);
	}
	else{
		UG_LOG("ERROR in ToolDialog::set_string_list: Parameter " << paramIndex << " can't be converted to a list box.\n");
		return false;
	}

	return true;
}

void ToolWidget::refreshContents()
{
	if(_tool)
		_tool->refresh_dialog(this);
}
