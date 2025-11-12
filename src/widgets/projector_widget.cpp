/*
 * Copyright (c) 2016:  G-CSC, Goethe University Frankfurt
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

#include <QListWidget>
#include <QComboBox>
#include <QEvent>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <typeinfo>
#include "projector_widget.hpp"
#include "promesh_plugin.h"
#include "../scene/lg_scene.hpp"

#include "tooldlg_oarchive.hpp"
#include "tooldlg_iarchive.hpp"
#include "common/boost_serialization_routines.h"
#include "common/util/archivar.h"
#include "common/util/factory.h"

// #include "lib_grid/boost_class_serialization_exports.h"
#include "lib_grid/refinement/projectors/projectors.h"

static ug::Factory<ug::RefinementProjector, ug::ProjectorTypes>	projFactory;

ProjectorWidget::
ProjectorWidget (QWidget* parent) :
	QFrame(parent),
	_cur_content(nullptr),
	_active_object(nullptr),
	_active_subset_index(-1)
{
	_vlayout = new QVBoxLayout(this);

	auto* hbox = new QHBoxLayout();
	_vlayout->addLayout(hbox);

	hbox->addWidget(new QLabel(tr("type:")));
	_type_box = new QComboBox(this);
	hbox->addWidget(_type_box);
	hbox->addStretch(2);

	_type_box->addItem(QString("none"));
	for(size_t iproj = 0; iproj < projFactory.num_classes(); ++iproj){
		_type_box->addItem(QString(projFactory.class_name(iproj).c_str()));
	}

	setEnabled(false);

	connect(_type_box, &QComboBox::currentIndexChanged, this, [this](int index) {
	QString text = _type_box->itemText(index);
	projectorTypeChanged(text);
});




}


void ProjectorWidget::
objectToBeRemoved(ISceneObject* pObj)
{
	if(pObj == static_cast<ISceneObject*>(_active_object))
		setActiveSubset(nullptr, -1);
}

void ProjectorWidget::
changeEvent(QEvent* evt)
{
	QFrame::changeEvent(evt);
	if(evt->type() == QEvent::EnabledChange){
		if(isEnabled()){
			_type_box->setVisible(true);
		}
		else{
			_type_box->setVisible(false);
			if(_cur_content)
				delete _cur_content;
			_cur_content = nullptr;
		}
	}
}


void ProjectorWidget::
setActiveSubset(ISceneObject* obj, int subsetIndex)
{
	_active_object = dynamic_cast<LGObject*>(obj);
	_active_subset_index = subsetIndex;
	if(_active_object && _active_subset_index >= 0){
		setEnabled(true);
		ug::SPRefinementProjector proj =
				_active_object->projection_handler().projector(subsetIndex);

		if(proj.valid()){
			QString itemText = QString(projFactory.class_name(*proj).c_str());
			int newIndex = _type_box->findText(itemText);
			if(newIndex != _type_box->currentIndex())
				_type_box->setCurrentIndex(newIndex);
			else
				update_content(proj.get());
		}
		else if(_type_box->currentIndex() != 0){
			_type_box->setCurrentIndex(0);
		}
	}
	else{
		setEnabled(false);
		if(_cur_content)
			delete _cur_content;
		_cur_content = nullptr;
	}
}

void ProjectorWidget::
projectorTypeChanged(const QString &text)
{
	if(!_active_object || _active_subset_index < 0)
		return;

	std::string projName = text.toStdString();
	if(projName.compare("none") == 0){
		_active_object->projection_handler()
			.set_projector(_active_subset_index, ug::SPRefinementProjector());
		update_content(nullptr);
	}
	else{
	//	if the current projector for the active subset has a different name, create a new one
		ug::SPRefinementProjector curProj =
				_active_object->projection_handler().projector(_active_subset_index);
		if(curProj.invalid() || projFactory.class_name(*curProj) != projName){
			ug::SPRefinementProjector proj = projFactory.create(projName);
			proj->set_geometry(_active_object->geometry());
			_active_object->projection_handler().set_projector(_active_subset_index, proj);
			// UG_LOG("created projector: " << typeid(*proj).name() << std::endl);
			update_content(proj.get());
		}
		else
			update_content(curProj.get());
	}
}

void ProjectorWidget::
update_content(ug::RefinementProjector* proj)
{
	if(!proj){
		if(_cur_content){
			delete _cur_content;
			_cur_content = nullptr;
		}
		return;
	}

	static ug::Archivar <ToolDlg_Oarchive,
						 ug::RefinementProjector,
						 ug::ProjectorTypes>
				archivar;

	ToolDlg_Oarchive oa(this);
	oa.set_expand_properties(true);
	
	archivar.archive(oa, *proj, "properties");

	QWidget* newContent = oa.widget();

	if(_cur_content)
		delete _cur_content;
	_cur_content = newContent;
	_vlayout->addWidget(_cur_content);

//	connect change-signals of all tool-widgets to contentChanged
	QList<ToolWidget*> toolWidgets = newContent->findChildren<ToolWidget*>();
	for(QList<ToolWidget*>::iterator iter = toolWidgets.begin();
		iter!= toolWidgets.end(); ++iter)
	{
		ToolWidget* tw = *iter;
		connect(tw, &ToolWidget::valueChanged, this, &ProjectorWidget::valueChanged);
	}
}

void ProjectorWidget::
valueChanged ()
{
	if(_cur_content && _active_object){
		static ug::Archivar <ToolDlg_Iarchive,
							 ug::RefinementProjector,
							 ug::ProjectorTypes>
				archivar;

		ug::SPRefinementProjector proj =
				_active_object->projection_handler().projector(_active_subset_index);

		ToolDlg_Iarchive ia(_cur_content);
		archivar.archive(ia, *proj);
	}
}