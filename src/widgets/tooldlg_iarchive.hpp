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

#ifndef __H__PROMESH_tooldlg_iarchive
#define __H__PROMESH_tooldlg_iarchive

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
#include <boost/archive/detail/common_iarchive.hpp>
#include <boost/archive/detail/register_archive.hpp>
#include "common/boost_serialization.h"

#include <QFrame>
#include <QVBoxLayout>

#include "common/error.h"

#include "extendible_widget.hpp"
#include "../tools/tool_dialog.hpp"

class ToolDlg_Iarchive :
		public boost::archive::detail::common_iarchive<ToolDlg_Iarchive>
{
public:
	using base_t = common_iarchive;
	using is_loading = boost::mpl::bool_<true>;
	using is_saving = boost::mpl::bool_<false>;

	ToolDlg_Iarchive() :
		_base(nullptr),
		_cur_name("")
	{}

	ToolDlg_Iarchive(QWidget* base) :
		_base(base),
		_cur_name("")
	{
		set_base_widget(base);
	}

	void set_base_widget(QWidget* base) {

		_widget_layers = std::stack<WidgetLayer> ();
		push_widget_layer(base);
	}

	void load_binary(const void *address, std::size_t count){}


	void load_object(
        void *x, 
        const boost::archive::detail::basic_iserializer & bis
    )
    {
    	if(_cur_name){
    		// std::cout << _prefix << "load_object: " << _cur_name << std::endl;
	    	push_child_layer();
	    	base_t::load_object(x, bis);
	    	pop_widget_layer();
	    }
	    else{
	    	base_t::load_object(x, bis);
	    }
    }

    const boost::archive::detail::basic_pointer_iserializer * 
    load_pointer(
        void * & t, 
        const boost::archive::detail::basic_pointer_iserializer * bpis_ptr,
        const boost::archive::detail::basic_pointer_iserializer * (*finder)(
            const boost::serialization::extended_type_info & eti))
    {
    	if(_cur_name){
    		// std::cout << _prefix << "load_pointer: " << _cur_name << std::endl;
	    	push_child_layer();
	    	const boost::archive::detail::basic_pointer_iserializer * 
	    		ret = base_t::load_pointer(t, bpis_ptr, finder);
	    	pop_widget_layer();
	    	return ret;
	    }
	    else{
	    	return base_t::load_pointer(t, bpis_ptr, finder);
	    }
    }



private:
	friend class interface_iarchive <ToolDlg_Iarchive>;
	friend class boost::archive::load_access;

	class WidgetLayer {
		public:
			explicit WidgetLayer (QWidget* base) :
				_base_widget(base),
				_cur_item_index(0),
				_cur_tool_index(0)
			{
			};

			QWidget* current_widget () {
				UG_COND_THROW(!_base_widget, "Current layer has no base widget. Bad input widget hierarchy.");
				UG_COND_THROW(!_base_widget->layout(), "Current widget has no layout. "
							  "Input widget sequence does not match input class.");
				UG_COND_THROW(_cur_item_index >= _base_widget->layout()->count(),
							  "Too many widgets requested from current layer. "
							  "Input widget sequence does not match input class.")
				return _base_widget->layout()->itemAt(_cur_item_index)->widget();
			}

			void next_widget ()		{++_cur_item_index;}

			int tool_index ()		{return _cur_tool_index;}
			void next_tool ()		{++_cur_tool_index;}

		private:
			QWidget*		_base_widget;
			int				_cur_item_index;
			int				_cur_tool_index;
	};


	ToolWidget* tool_widget () {
		ToolWidget* twdgt = dynamic_cast<ToolWidget*>(top_layer().current_widget());
		UG_COND_THROW(!twdgt, "ToolWidget expected, but other widget received. "
					  "Input widget sequence does not match input class.");
		return twdgt;
	}

	template<class T>
	void load_override (T &t){
		base_t::load_override(t);
	}

	template<class T>
	void load_override (const boost::serialization::nvp <T> &t){
		_cur_name = t.name();
		base_t::load_override(t);
	}

	template<class T>
	void load_override (T &t, int i){
		base_t::load_override(t, i);
	}

	template<class T>
	void load_override (const boost::serialization::nvp <T> &t, int i){
		_cur_name = t.name();
		base_t::load_override(t, i);
	}

	template<class T>
	void load (T &t){
		// std::cout << _prefix << "load unknown\n";
	}

	void load (bool& val){
		bool ok = false;
		val = tool_widget()->to_bool(top_layer().tool_index(), &ok);
		UG_COND_THROW(!ok, "conversion to bool failed. "
					  "Input widget sequence does not match input class");
		top_layer().next_tool();
		// std::cout << _prefix << "load " << _cur_name << ": " << val << std::endl;
	}

	void load (int& val){
		bool ok = false;
		val = tool_widget()->to_int(top_layer().tool_index(), &ok);
		UG_COND_THROW(!ok, "conversion to int failed. "
					  "Input widget sequence does not match input class");
		top_layer().next_tool();
		// std::cout << _prefix << "load " << _cur_name << ": " << val << std::endl;
	}

	void load (float& val){
		bool ok = false;
		val = tool_widget()->to_double(top_layer().tool_index(), &ok);
		UG_COND_THROW(!ok, "conversion to float failed. "
					  "Input widget sequence does not match input class");
		top_layer().next_tool();
		// std::cout << _prefix << "load " << _cur_name << ": " << val << std::endl;
	}

	void load (double& val){
		bool ok = false;
		val = tool_widget()->to_double(top_layer().tool_index(), &ok);
		UG_COND_THROW(!ok, "conversion to double failed. "
					  "Input widget sequence does not match input class");
		top_layer().next_tool();
		// std::cout << _prefix << "load " << _cur_name << ": " << val << std::endl;
	}

	void load (std::string& val){
		bool ok = false;
		val = tool_widget()->to_string(top_layer().tool_index(), &ok).toStdString();
		UG_COND_THROW(!ok, "conversion to string failed. "
					  "Input widget sequence does not match input class");
		top_layer().next_tool();
		// std::cout << _prefix << "load " << _cur_name << ": " << val << std::endl;
	}

	#ifndef BOOST_NO_STD_WSTRING
	void load (std::wstring &ws){
	}
	#endif

	
	void push_child_layer ()
	{
		if(dynamic_cast<ToolWidget*>(top_layer().current_widget()))
			top_layer().next_widget();

		ExtendibleWidget* ewgt = dynamic_cast<ExtendibleWidget*>(top_layer().current_widget());
		UG_COND_THROW(!ewgt, "ExtendibleWidget expected, but other widget received. "
					  "Input widget sequence does not match input class.");
		top_layer().next_widget();
		push_widget_layer (ewgt->widget());
	}

	void push_widget_layer (QWidget* base)
	{
		UG_COND_THROW (!base, "No base widget was set. Please do so through "
						"'tooldlg_iarchive::set_base_widget(...)'");
		_prefix.append ("  ");
		_widget_layers.emplace(base);
	}

	void pop_widget_layer ()
	{
		if(!_widget_layers.empty()){
			_widget_layers.pop();
		}
		if(_prefix.size() > 2)
			_prefix.resize (_prefix.size() - 2);
	}

	WidgetLayer& top_layer ()	{
		UG_COND_THROW(_widget_layers.empty(), "No widget layer available! "
					  "Input widget sequence does not match input class.");
		return _widget_layers.top();
	}


	QWidget*		_base;
	std::stack<WidgetLayer>	_widget_layers;
	const char*		_cur_name;	///< can be nullptr due to intermediate objects
	std::string		_prefix;
};

BOOST_SERIALIZATION_REGISTER_ARCHIVE(ToolDlg_Iarchive);

template <>
struct ug::ArchiveInfo <ToolDlg_Iarchive> {
	static constexpr ArchiveType TYPE = AT_GUI;
};//	end of namespace ug

#endif	//__H__UG_tooldlg_iarchive
