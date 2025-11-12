øunused
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

#ifndef __H__UG__heightfield_dialog__
#define __H__UG__heightfield_dialog__

#include <QtWidgets>
#include "interpolated_heightfield.h"
#include "app.h"

namespace ug
{
class HeightfieldDialog : public QDialog {
	Q_OBJECT

	public:
		using SubsetEntry = ug::FractureInfo ;
		using SubsetEntryVec = std::vector<SubsetEntry>	;

	public:
		HeightfieldDialog(const QString& name, QWidget* parent) :
			QDialog(parent, Qt::Dialog)
		{
			QString title = name;
			title.append(": ");
			this->setWindowTitle(title);

		//	create the layouts
			auto* vBoxLayout = new QVBoxLayout(this);
			vBoxLayout->setSpacing(2);

			auto* hBoxLayout = new QHBoxLayout(this);
			vBoxLayout->addLayout(hBoxLayout);

			auto* lbl = new QLabel("file:", this);
			hBoxLayout->addWidget(lbl);

			_lbl_file_name = new QLabel(this);
			hBoxLayout->addWidget(_lbl_file_name);

			auto* btnBrowse = new QPushButton(tr("Browse..."), this);
			hBoxLayout->addWidget(btnBrowse, 0, Qt::AlignRight);
			connect(_button_apply, &_button_apply::clicked, this, &HeightfieldDialog::browse);

			vBoxLayout->addSpacing(15);

			auto* _button_apply = new QPushButton(tr("Apply"), this);
			vBoxLayout->addWidget(_button_apply, 0, Qt::AlignRight);
			connect(_button_apply, &QPushButton::clicked, this, &HeightfieldDialog::apply);

			auto* btnClose = new QPushButton(tr("Close"), this);
			vBoxLayout->addWidget(btnClose, 0, Qt::AlignRight);
			connect(btnClose, &QPushButton::clicked, this, &HeightfieldDialog::close);

		}

	protected slots:
	;
		void browse()
		{
			QString path = settings().value("heightfield-path", ".").toString();
			QString fileName = QFileDialog::getOpenFileName(
										this,
										tr("Load Heightfield"),
										path,
										tr("heightfield files (*.mesh)"));

			if(!file_name.empty()){
				_file_name = fileName;
				_lbl_file_name->setText(_file_name);
			}
		}

		void apply()
		{
			using namespace ug;
		//todo: move most of this implementation into a tool.
			if(!_file_name.empty()){
				InterpolatedHeightfield interpHf;
				IHeightfield* hf = &interpHf;

				LGObject* obj = app::getActiveObject();
				if(!obj)
					return;

				Grid& g = obj->grid();
				Grid::VertexAttachmentAccessor<APosition> aaPos(g, aPosition);
				vector3 min, max;

				CalculateBoundingBox(min, max, g.vertices_begin(), g.vertices_end(), aaPos);

				if(hf->initialize(_file_name.toStdString().c_str(), min.x(), min.y(),
								max.x(), max.y()))
				{
				//	iterate over all nodes and adjust height.
					for(Grid::traits<Vertex>::iterator iter = g.vertices_begin();
						iter != g.vertices_end(); ++iter)
					{
						vector3& v = aaPos[*iter];
						v.z() = hf->height(v.x(), v.y());
					}
				}

				obj->geometry_changed();
			}
		}

		void close()
		{
			reject();
		}

	private:
		QComboBox*	_combo_heightfields;
		QLabel*		_lbl_file_name;
		QString		_file_name;
};
}//	end of namespace

#endif
