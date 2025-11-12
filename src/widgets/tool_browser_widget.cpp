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
 
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "icon_tab_widget.hpp"
#include "tool_browser_widget.hpp"
#include "widget_container.hpp"
#include "widget_list.hpp"
#include "extendible_widget.hpp"

using namespace std;

ToolBrowser::ToolBrowser(QWidget* parent) :
	QFrame(parent),
	_icon_tab(nullptr),
	_revision(0)
{
	_layout = new QVBoxLayout(this);
	_layout->setSpacing(0);
	_layout->setContentsMargins(0, 0, 0, 0);
	this->setLayout(_layout);

//	connect the signal mapper to the launchTool slot.
	//æ connect(m_signalMapper, SIGNAL(mapped(int)), this, &ToolBrowser::executeTool); // args: int
}


WidgetContainer* ToolBrowser::group_container(const std::string& groupName)
{
	vector<string> groupTokens;
	ug::TokenizeTrimString(groupName, groupTokens, '|');

	UG_COND_THROW(groupTokens.empty(), "Invalid group name: '" << groupName << "'");

	string curGrp = groupTokens[0];
	WidgetContainer* parent = _group_containers[curGrp];
	if(parent == nullptr){
		auto* wlist = new WidgetList(_icon_tab);
		parent = wlist->widgetContainer();
		_icon_tab->addPage(wlist, _tool_mgr->group_icon(curGrp), QString(curGrp.c_str()));
		_group_containers[curGrp] = parent;
	}

	for(size_t igrp = 1; igrp < groupTokens.size(); ++igrp){
		curGrp.append("|").append(groupTokens[igrp]);
		WidgetContainer* curContainer = _group_containers[curGrp];
		if(curContainer == nullptr){
			auto* extWidget = new ExtendibleWidget(parent);
			parent->addWidget(extWidget);
			QString extWidgetName(groupTokens[igrp].c_str());
			QString infoText("Group: ");
			infoText.append(extWidgetName);
			extWidgetName.append(" ...");
			extWidget->setText(extWidgetName);
			extWidget->setInfoText(infoText);
			curContainer = new WidgetContainer(extWidget);
			extWidget->setWidget(curContainer);
			_group_containers[curGrp] = curContainer;
		}

		parent = curContainer;
	}

	return parent;
}

void ToolBrowser::refresh(ToolManager* toolMgr)
{
	if(_tool_mgr && (_tool_mgr != toolMgr)){
		delete _icon_tab;
		_icon_tab = nullptr;
		_tool_map = map<string, ToolEntry>();
		_group_containers = map<string, WidgetContainer*>();
	}

	_tool_mgr = toolMgr;
	if(!_tool_mgr)
		return;

	if(!_icon_tab){
		_icon_tab = new IconTabWidget(this);
		_layout->addWidget(_icon_tab);
	}

//	add empty groups to preserve the group order
	for(size_t i = 0; i < toolMgr->num_known_groups(); ++i)
		group_container(toolMgr->known_group(i));


//	the revision is used to identify unused tool-entries
	++_revision;

//	iterate through all tools of the toolMgr and create or adjust entries in m_toolMap
	for(size_t itool = 0; itool < toolMgr->num_tools(); ++itool){
		ITool* tool = toolMgr->tool(itool);
		string name = tool->get_group();
		name.append(tool->get_name());

		ToolEntry& entry = _tool_map[name];

	//	first we'll check whether the tool with the given name changed
		if(entry._tool && (entry._tool != tool)){
		//	do some cleanup
			delete entry._widget;
			if(entry._extendible_widget)
				delete entry._extendible_widget;
			entry._widget = nullptr,
			entry._extendible_widget = nullptr;
			entry._tool = nullptr;
			entry._parent_container = nullptr;
		}

		if(!entry._tool){
		//	we'll populate a new entry
			entry._tool = tool;
			entry._parent_container = group_container(string(tool->get_group()));
			auto* extWidget = new ExtendibleWidget(entry._parent_container);
			entry._widget = tool->get_dialog(extWidget);
			if(entry._widget){
			//	create an extendible widget and add w into it
				extWidget->setWidget(entry._widget);
				extWidget->setText(tool->get_name());
				extWidget->setInfoText(tr(tool->get_tooltip()));
				entry._extendible_widget = extWidget;
				entry._parent_container->addWidget(extWidget);
			}
			else{
			//	create a command button and connect it to the given tool
				delete extWidget;
				auto* btn = new QPushButton(tool->get_name(), entry._parent_container);
				btn->setToolTip(tr(tool->get_tooltip()));

				connect(btn, &QPushButton::clicked, this, [this,itool](){this->executeTool((int)itool);}); //ø arg: none
				entry._widget = btn;
				entry._parent_container->addWidget(entry._widget);
			}
		}
		else{
		//	if the tool-widget changed, we'll have to replace it
			if(tool->dialog_changed(entry._widget)){
				delete entry._widget;
				ExtendibleWidget* extWidget = entry._extendible_widget;
				if(!extWidget)
					extWidget = new ExtendibleWidget(entry._parent_container);

				entry._widget = tool->get_dialog(extWidget);
				if(entry._widget){
					extWidget->setWidget(entry._widget);
					extWidget->setText(tool->get_name());
					extWidget->setInfoText(tr(tool->get_tooltip()));
					if(!entry._extendible_widget){
						entry._extendible_widget = extWidget;
						entry._parent_container->addWidget(entry._extendible_widget);
					}
				}
				else{
					delete extWidget;
					entry._extendible_widget = nullptr;
					auto* btn = new QPushButton(tool->get_name(), entry._parent_container);
					btn->setToolTip(tr(tool->get_tooltip()));
					//connect(btn, &QPushButton::clicked, this, &ToolBrowser::itool); //ø none
					connect(btn, &QPushButton::clicked, this, [this,itool](){this->executeTool((int)itool);}); //ø arg: none
					entry._widget = btn;
					entry._parent_container->addWidget(entry._widget);
				}
			}
		}
		entry._revision = _revision;
	}

//	iterate over all entries and check for each whether its revision matches
//	the current revision. If not, we'll invalidate the entry and delete associated widgets.
	for(auto iter = _tool_map.begin(); iter != _tool_map.end(); ++iter)
	{
		ToolEntry& entry = iter->second;
		if(entry._revision != _revision){
			if(entry._widget)
				delete entry._widget;
			if(entry._extendible_widget)
				delete entry._extendible_widget;
			entry = ToolEntry();
		}
	}

//	finally search for empty groups and delete those. This has to be performed
//	recursively
	for(int itab = 0; itab < _icon_tab->count(); ++itab){
		auto* wlist = qobject_cast<WidgetList*>(_icon_tab->widget(itab));
		deleteEmptyChildGroups(wlist->widgetContainer());
	}
}

void ToolBrowser::deleteEmptyChildGroups(QWidget* w)
{
//	we only consider extendible widgets containing widget-containers
//todo: the search through _group_containers could be a little expensive if one
//		would change many tools at once (e.g. move the whole scrip path).
//		One could think about storing the group-name inside WidgetContainer,
//		so that the entry in _group_containers could be easily accessed.
	QList<ExtendibleWidget*> list = w->findChildren<ExtendibleWidget*>(QString(), Qt::FindDirectChildrenOnly);
	for(auto i = list.begin(); i != list.end(); ++i){
		ExtendibleWidget* ew = *i;
		auto* wc = qobject_cast<WidgetContainer*>(ew->widget());
		if(wc){
			deleteEmptyChildGroups(wc);
			if(!wc->findChild<QWidget*>(QString(), Qt::FindDirectChildrenOnly)){
			//	we have to find the entry in _group_containers that holds wc and remove it
				for(auto giter = _group_containers.begin(); giter != _group_containers.end(); ++giter)
				{
					if(giter->second == wc){
						_group_containers.erase(giter);
						break;
					}
				}
				delete ew;
			}
		}
	}
}

void ToolBrowser::executeTool(int toolID)
{
	_tool_mgr->launchTool(toolID);
}
