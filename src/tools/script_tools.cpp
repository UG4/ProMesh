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

#include <QDir>
#include <QApplication>
#include <QFile>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <sstream>

#include "script_tools.hpp"
#include "standard_tools.hpp"
#include "common/error.h"
#include "common/util/string_util.h"
#include "../app.hpp"
#include "../../plugins/LuaShell/lua_shell.h"
#include "bindings/lua/lua_util.h"
#include "common/error.h"

using namespace ug;
using namespace std;

class ScriptTool;
static vector<ScriptTool*>	g_scriptTools;

template <typename T>
static T ToNumber(const std::string& str){
	std::istringstream istr(str.c_str());
	istr.imbue(std::locale("C"));
	T num = 0;
	istr >> num;
	return num;
}


ScriptTool::
ScriptTool(string path, string group, SPLuaShell luaShell) :
	_script_name(path),
	_script_path(path),
	_group(group),
	_lua_shell(luaShell)
{
}

void ScriptTool::
execute(LGObject* obj, QWidget* widget)
{
	auto* dlg = dynamic_cast<ToolWidget*>(widget);
	if(dlg){
		for(size_t i = 0; i < _script_decls._inputs.size(); ++i){
			ScriptParameter& param = _script_decls._inputs[i];
			if((param._type_name == "double") || (param._type_name == "float")){
				_lua_shell->set(param._var_name.c_str(), dlg->to_double((int)i));
			}
			else if((param._type_name == "int") || (param._type_name == "integer")){
				_lua_shell->set(param._var_name.c_str(), dlg->to_int((int)i));
			}
			else if((param._type_name == "bool") || (param._type_name == "boolean")){
				_lua_shell->set(param._var_name.c_str(), dlg->to_bool((int)i));
			}
			else if(param._type_name == "string"){
				_lua_shell->set(param._var_name.c_str(), dlg->to_string((int)i).toLocal8Bit().constData());
			}
			else{
				UG_THROW("type " << param._type_name << " currently unsupported by script interpreter.");
			}
		}
	}

	try{
		_lua_shell->set("mesh", static_cast<ug::promesh::Mesh*>(obj), "Mesh");
		_lua_shell->run(_script_content.constData());
	}
	catch(ug::script::LuaError& err) {
		ug::PathProvider::clear_current_path_stack();
		if(err.show_msg()){
			if(!err.get_msg().empty()){
				UG_LOG("error in script " << _script_name << "(file: " << _script_path << "):\n");
				for(size_t i=0;i<err.num_msg();++i)
					UG_LOG(err.get_msg(i)<<endl);
			}
		}
	}

	obj->geometry_changed();
}

const char* ScriptTool::
get_name()	
{
	parse_script_decls(false);
	return _script_decls._name.c_str();
}

const char* ScriptTool::
get_tooltip()
{return "";}

const char* ScriptTool::
get_group()
{return _group.c_str();}

const char* ScriptTool::
get_shortcut()
{return "";}


bool ScriptTool::
accepts_null_object_ptr()	{return false;}

QWidget* ScriptTool::
get_dialog(QWidget* parent)
{
	parse_script_decls(true);
	
	vector<ScriptParameter>& inputs = _script_decls._inputs;

	if(inputs.empty())
		return nullptr;

	auto *dlg = new ToolWidget(get_name(), parent, this, IDB_APPLY | IDB_OK | IDB_CLOSE);

	refresh_dialog(dlg);

	return dlg;
}

/** \todo: this method should check whether there were indeed any changes
*		   to the parameters of the script.*/
bool ScriptTool::
dialog_changed(QWidget* dlg)		{return true;}

void ScriptTool::
refresh_dialog(QWidget* dialog)
{
	auto* dlg = dynamic_cast<ToolWidget*>(dialog);
	if(!dlg)
		return;

	parse_script_decls(true);
	dlg->clear();
	
	vector<ScriptParameter>& inputs = _script_decls._inputs;

	if(inputs.empty())
		return;

	for(size_t iinput = 0; iinput < inputs.size(); ++iinput){
		ScriptParameter& param = inputs[iinput];
		if((param._type_name == "double") || (param._type_name == "float")){
			dlg->addSpinBox (QString(param._arg_name.c_str()),
			                 param._min.to_double(),
			                 param._max.to_double(),
			                 param._val.to_double(),
			                 param._step.to_double(),
			                 param._digits.to_double());
		}

		else if((param._type_name == "int") || (param._type_name == "integer")){
			dlg->addSpinBox (QString(param._arg_name.c_str()),
			                 param._min.to_int(),
							 param._max.to_int(),
							 param._val.to_int(),
							 param._step.to_int(),
							 0);
		}

		else if((param._type_name == "bool") || (param._type_name == "boolean")){
			dlg->addCheckBox(QString(param._arg_name.c_str()), param._val.to_bool());
		}

		else if(param._type_name == "string"){
			dlg->addTextBox(QString(param._arg_name.c_str()),
			                QString(param._val.to_c_string()));
		}
	}
}

void ScriptTool::
parse_script_decls(bool force){
	if(force || _script_decls._name.empty()){
		QFile file(QString(_script_path.c_str()));
		if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
			_script_content = QByteArray();
			_script_decls = ScriptDeclarations();
			UG_LOG("ERROR: Failed to read script " << _script_path << endl);
		}
		else{
			_script_content = file.readAll();
			file.close();
			ParseScriptDeclarations(_script_decls,
			                        _script_content.constData());
		}
		if(_script_decls._name.empty())
			_script_decls._name = _script_name;
	}
}

string ScriptTool::
path()			{return _script_path;}

SPLuaShell ScriptTool::
lua_shell()	{return _lua_shell;}




void ParseDirAndCreateTools(ToolManager* toolMgr, QDir dir, string group,
							SPLuaShell luaShell,
							bool performExistenceChecks = false)
{
//	parse subdirectories first
	QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	for(int idir = 0; idir < subDirs.size(); ++idir){
		// UG_LOG(group << subDirs.at(idir).toStdString() << endl);
		
		QDir subDir = dir;
		subDir.cd(subDirs.at(idir));
		
		string subGroup = group;
		if(!subGroup.empty())
			subGroup.append(" | ");
		subGroup.append(subDirs.at(idir).toStdString());
		
		ParseDirAndCreateTools(toolMgr, subDir, subGroup, luaShell, performExistenceChecks);
	}

	QStringList scripts = dir.entryList(QDir::Files);
	for(int iscript = 0; iscript < scripts.size(); ++iscript){
		// UG_LOG(group << "* "<< scripts.at(iscript).toStdString() << endl);

	//	check whether the given tool already exists
		string absPath = dir.absoluteFilePath(scripts.at(iscript)).toStdString();
		if(performExistenceChecks){
			bool gotOne = false;
			for(size_t i = 0; i < g_scriptTools.size(); ++i){
				if(g_scriptTools[i]->path() == absPath){
					gotOne = true;
					break;
				}
			}
			if(gotOne)
				continue;
		}

		auto* tool = new ScriptTool(absPath, group, luaShell);

		toolMgr->register_tool(tool);
		g_scriptTools.push_back(tool);
	}
}

void RegisterScriptTools(ToolManager* toolMgr)
{
	SmartPtr<luashell::LuaShell> shell = make_sp(new luashell::LuaShell());
	ParseDirAndCreateTools(toolMgr, QDir(":/scripts"), "Scripts", shell);
	ParseDirAndCreateTools(toolMgr, app::UserScriptDir(), "Scripts", shell);

	std::vector<QDir> vDirs = app::CustomUserScriptDirs();

	for(int i = 0; i < vDirs.size(); ++i){
		QString scriptGroupName = "Scripts | " + vDirs[i].dirName();
		ParseDirAndCreateTools(toolMgr, vDirs[i],
				scriptGroupName.toLocal8Bit().constData(), shell);
	}
}

int RefreshScriptTools(ToolManager* toolMgr)
{
	size_t lastNumScriptTools = g_scriptTools.size();
	int retVal = 0;

//	check for each tool, whether the according script still exists
	for(size_t i = 0; i < g_scriptTools.size();){
		ScriptTool* t = g_scriptTools[i];
		QFile file(QString(t->path().c_str()));
		if(!file.exists()){
			toolMgr->remove_tool(t);
			delete t;
			g_scriptTools.erase(g_scriptTools.begin() + i);
		}
		else{
			++i;
		}
	}

	if(lastNumScriptTools != g_scriptTools.size())
		retVal = 1;

	lastNumScriptTools = g_scriptTools.size();

	SPLuaShell luaShell;
	luaShell = GetDefaultLuaShell();

	ParseDirAndCreateTools(toolMgr, QDir(":/scripts"), "Scripts", luaShell, true);
	ParseDirAndCreateTools(toolMgr, app::UserScriptDir(), "Scripts", luaShell, true);

	std::vector<QDir> vDirs = app::CustomUserScriptDirs();

	for(int i = 0; i < vDirs.size(); ++i){
		QString scriptGroupName = "Scripts | " + vDirs[i].dirName();
		ParseDirAndCreateTools(toolMgr, vDirs[i],
				scriptGroupName.toLocal8Bit().constData(), luaShell, true);
	}

	if(lastNumScriptTools != g_scriptTools.size())
		retVal = 1;

	return retVal;
}
