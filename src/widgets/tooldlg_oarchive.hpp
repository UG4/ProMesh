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

#ifndef __H__PROMESH_tooldlg_oarchive
#define __H__PROMESH_tooldlg_oarchive

#include <iostream>
#include <stack>
#include <boost/type_traits/is_enum.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/archive/detail/common_oarchive.hpp>
#include <boost/archive/detail/register_archive.hpp>
#include <boost/archive/impl/archive_serializer_map.ipp>
#include "common/boost_serialization.h"

#include <QFrame>
#include <QVBoxLayout>

#include "extendible_widget.hpp"
#include "../tools/tool_dialog.hpp"

class ToolDlg_Oarchive :
		public boost::archive::detail::common_oarchive<ToolDlg_Oarchive>
{
public:
	using base_t = common_oarchive;
	using is_loading = boost::mpl::bool_<false>;
	using is_saving = boost::mpl::bool_<true>;

	ToolDlg_Oarchive(QWidget* parent) :
		_parent(parent),
		_layout(nullptr),
		_cur_name(""),
		_expand_properties(false)
	{
		push_widget_layer();
	}

	void set_expand_properties(bool expand)	{_expand_properties = expand;}
	
	QWidget* widget()
	{
		if(_widget_layers.empty())
			push_widget_layer();
		return _widget_layers.top().frame();
	}

	void save_binary(const void *address, std::size_t count){}


	void save_object(
        const void *x, 
        const boost::archive::detail::basic_oserializer & bos
    )
    {
    	if(_cur_name){
    		// std::cout << _prefix << "save_object: " << _cur_name << std::endl;
	    	push_widget_layer(_cur_name);
	    	base_t::save_object(x, bos);
	    	pop_widget_layer();
	    }
	    else{
	    	base_t::save_object(x, bos);
	    }
    }

	void save_pointer(
		const void * t, 
		const boost::archive::detail::basic_pointer_oserializer * bpos_ptr)
    {
    	if(_cur_name){
    		// std::cout << _prefix << "save_pointer: " << _cur_name << std::endl;
	    	push_widget_layer(_cur_name);
	    	base_t::save_pointer(t, bpos_ptr);
	    	pop_widget_layer();
	    }
	    else{
	    	base_t::save_pointer(t, bpos_ptr);
	    }
    }



private:
	friend class interface_oarchive;
	friend class boost::archive::save_access;

	ToolWidget* tool_widget(const char* name) {
		if(_widget_layers.empty())
			push_widget_layer("");
		return _widget_layers.top().toolWidget();
	}

	template <class T>
	void save_override(const T &t) {
		base_t::save_override(t);
	}

	template<class T>
	void save_override(const boost::serialization::nvp <T> &t){
		_cur_name = t.name();
		base_t::save_override(t);
	}


	template<class T>
	void save_override(const T &t, int i){
		base_t::save_override(t, i);
	}

	template<class T>
	void save_override(const boost::serialization::nvp <T> &t, int i){
		_cur_name = t.name();
		base_t::save_override(t, i);
	}

	template<class T>
	void save(const T &t){
	}

	void save(bool val){
		// std::cout << _prefix << "save " << _cur_name << ": " << val << std::endl;
		tool_widget(_cur_name)->addCheckBox(QString(_cur_name).append(":"), val);
	}

	void save(int val){
		// std::cout << _prefix << "save " << _cur_name << ": " << val << std::endl;
		create_spinner<int>(-1.e9, 1.e9, val, 1, 0);
	}

	void save(float val){
		// std::cout << _prefix << "save " << _cur_name << ": " << val << std::endl;
		create_spinner<float>(-1.e9, 1.e9, val, 1, 6);
	}

	void save(double val){
		// std::cout << _prefix << "save " << _cur_name << ": " << val << std::endl;
		create_spinner<float>(-1.e9, 1.e9, val, 1, 6);
	}

	template <class T>
	void create_spinner(T min, T max, T value, T step, T digits)
	{
		tool_widget(_cur_name)->addSpinBox(QString(_cur_name).append(":"), min, max, value, step, digits);
	}

	void save(const std::string& val){
		// std::cout << _prefix << "save " << _cur_name << ": " << val << std::endl;
		tool_widget(_cur_name)->addTextBox(QString(_cur_name).append(":"), QString(val.c_str()));
	}

	#ifndef BOOST_NO_STD_WSTRING
	void save(const std::wstring &ws){
	}
	#endif

	void push_widget_layer(const char* name = 0)
	{
		QWidget* parent = _parent;
		QLayout* layout = _layout;
		if(!_widget_layers.empty()){
			parent = _widget_layers.top().frame();
			layout = _widget_layers.top().layout();
		}

		ExtendibleWidget* extWidget = nullptr;
		if(name != nullptr && *name != 0){
			extWidget = new ExtendibleWidget(parent);
			layout->addWidget(extWidget);
			layout->setAlignment(extWidget, Qt::AlignLeft);
			extWidget->setText(QString(name));
			extWidget->setChecked(_expand_properties);
			parent = extWidget;
			layout = nullptr;
		}

		_widget_layers.emplace(parent);

		if(extWidget)
			extWidget->setWidget(_widget_layers.top().frame());

		_prefix.append("  ");
	}

	void pop_widget_layer()
	{
		if(!_widget_layers.empty()){
			_widget_layers.pop();
		}
		if(_prefix.size() > 2)
			_prefix.resize(_prefix.size() - 2);
	}

	class WidgetLayer {
		public:
			WidgetLayer (QWidget* parent) :
				_tool_widget(nullptr)
			{
				_frame = new QFrame(parent);
				_layout = new QVBoxLayout(_frame);
				_layout->setSpacing(0);
				_layout->setContentsMargins(0, 0, 0, 0);
				_frame->setLayout(_layout);
				_frame->setLineWidth(1);
			};

			QFrame* frame () {return _frame;}
			QLayout* layout () {return _layout;}
			ToolWidget* toolWidget(){
				if(!_tool_widget){
					_tool_widget = new ToolWidget(QString("--properties--"), _frame, nullptr, 0);
					_layout->addWidget(_tool_widget);
					_layout->setAlignment(_tool_widget, Qt::AlignLeft);
				}
				return _tool_widget;
			}

		private:
			QFrame* _frame;
			QLayout* _layout;
			ToolWidget* _tool_widget;
	};

	QWidget* _parent;
	QFrame* _frame;
	std::stack<WidgetLayer>	_widget_layers;
	QLayout* _layout;
	const char* _cur_name;	///< can be nullptr due to intermediate objects
	std::string _prefix;
	bool _expand_properties;
};

BOOST_SERIALIZATION_REGISTER_ARCHIVE(ToolDlg_Oarchive);

template <>
struct ug::ArchiveInfo <ToolDlg_Oarchive> {
	static constexpr ArchiveType TYPE = AT_GUI;
};//	end of namespace ug


#endif