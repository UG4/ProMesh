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
#include "delegates.hpp"

VisibilityDelegate::VisibilityDelegate(int myColumn,
					QObject* parent) : QStyledItemDelegate(parent)
{
	_column = myColumn;
	_image_visible.load(":images/visible_16.png");
	_image_invisible.load(":images/invisible_16.png");
}

QSize VisibilityDelegate::sizeHint(const QStyleOptionViewItem & option,
					   const QModelIndex & index ) const
{
	if(index.column() == _column)
		return QSize(16, 16);
	return QSize();
}

void VisibilityDelegate::paint(QPainter* painter,
				   const QStyleOptionViewItem& option,
				   const QModelIndex& index) const
{
	if(index.column() == _column)
	{
	//	calculate the center
		int cX = (option.rect.left() + option.rect.right()) / 2;
		int cY = (option.rect.bottom() + option.rect.top()) / 2;

		bool visible = index.model()->data(index, SIDR_VISIBLE).toBool();
		if(visible)
			painter->drawImage(QRect(cX - 8, cY - 8, 16, 16),
							   _image_visible);
		else
			painter->drawImage(QRect(cX - 8, cY - 8, 16, 16),
							   _image_invisible);
	}
	else
		QStyledItemDelegate::paint(painter, option, index);
}

QWidget* VisibilityDelegate::createEditor(QWidget* parent,
							  const QStyleOptionViewItem& option,
							  const QModelIndex& index) const
{
	return nullptr;
}


////////////////////////////////////////////////////////////////////////
ColorDelegate::ColorDelegate(int myColumn, int myRole,
					QObject* parent) : QStyledItemDelegate(parent)
{
	_column = myColumn;
	_role = myRole;
}

QSize ColorDelegate::sizeHint(const QStyleOptionViewItem & option,
					   const QModelIndex & index ) const
{
	if(index.column() == _column)
		return QSize(16, 16);
	return QSize();
}

void ColorDelegate::paint(QPainter* painter,
				   const QStyleOptionViewItem& option,
				   const QModelIndex& index) const
{
	if(index.column() == _column)
	{
	//	calculate the center
		int cX = (option.rect.left() + option.rect.right()) / 2;
		int cY = (option.rect.bottom() + option.rect.top()) / 2;

		bool ok = false;
		uint col = index.model()->data(index, _role).toUInt(&ok);
		if(ok)
		{
			painter->setBrush(QBrush(col, Qt::SolidPattern));
			painter->drawRect(QRect(cX - 7, cY - 7, 14, 14));
		}

	}
	else
		QStyledItemDelegate::paint(painter, option, index);
}

QWidget* ColorDelegate::createEditor(QWidget* parent,
							  const QStyleOptionViewItem& option,
							  const QModelIndex& index) const
{
	bool ok = false;
	uint col = index.model()->data(index, _role).toUInt(&ok);
	if(ok)
	{
		auto* editor = new QColorDialog(col, parent);
		connect(editor, &QColorDialog::colorSelected, this, &ColorDelegate::updateColorAndQuitEditor);

		return editor;
	}
	return nullptr;
}

void ColorDelegate::setEditorData(QWidget* editor,
						   const QModelIndex& index) const
{
}

void ColorDelegate::setModelData(QWidget* editor,
						  QAbstractItemModel* model,
						  const QModelIndex & index) const
{
	auto* colEdit = qobject_cast<QColorDialog*>(editor);
	model->setData(index, QVariant(colEdit->currentColor().rgb()), _role);
}

void ColorDelegate::updateColorAndQuitEditor(const QColor & color)
{
	auto* editor = qobject_cast<QColorDialog*>(sender());
	emit commitData(editor);
	emit closeEditor(editor);
}
