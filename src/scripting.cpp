/*
 * Copyright (c) 2017:  G-CSC, Goethe University Frankfurt
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

#include <limits>
#include <QFile>
#include "util/file_util.hpp"
#include "scripting.hpp"

using namespace std;
using namespace ug;


static SPLuaShell g_luaShell;


template <typename T>
static T ToNumber(const std::string& str){
	std::istringstream istr(str);
	istr.imbue(std::locale("C"));
	T num = 0;
	istr >> num;
	return num;
}


SPLuaShell GetDefaultLuaShell ()
{
	if(!g_luaShell.valid()){
		g_luaShell = make_sp(new luashell::LuaShell());
	}
	return g_luaShell;
}


void SetScriptDefaultVariables (SPLuaShell luaShell, const char* scriptContent)
{
	ScriptDeclarations decls;
	ParseScriptDeclarations (decls, scriptContent);

	for(size_t i = 0; i < decls._inputs.size(); ++i){
		ScriptParameter& param = decls._inputs[i];

		switch(param._val.type()) {
			case Variant::VT_BOOL:
				luaShell->set (param._var_name.c_str(), param._val.to_bool());
				break;
			case Variant::VT_INT:
				luaShell->set (param._var_name.c_str(), param._val.to_int());
				break;
			case Variant::VT_SIZE_T:
				luaShell->set (param._var_name.c_str(), param._val.to_size_t());
				break;
			case Variant::VT_FLOAT:
			case Variant::VT_DOUBLE:
				luaShell->set (param._var_name.c_str(), param._val.to_double());
				break;
			case Variant::VT_STDSTRING:
			case Variant::VT_CSTRING:
				luaShell->set (param._var_name.c_str(), param._val.to_c_string());
				break;
		}
	}
}


void ParseScriptDeclarations (ScriptDeclarations& declsOut,
                              const char* scriptContent)
{
	ScriptDeclarations& decls = declsOut;
	decls._name = "";
	decls._inputs.clear();
	std::stringstream in(scriptContent);

	string line;
	vector<string> tokens;
	vector<string> paramTokens;
	int curLineNumber = 0;
	

	while(!in.eof()){
		++curLineNumber;

		getline(in, line);

		if(!line.empty() && line[line.size() - 1] == '\r')
			line.resize(line.size() - 1);

		if(line.find("pm-declare-") == string::npos)
			continue;

		TokenizeTrimString(line, tokens, ':');
		if(tokens.size() != 2){
			continue;
		}

		RemoveWhitespaceFromString(tokens[0]);
		

		if(tokens[0].compare("--pm-declare-name") == 0){
			decls._name = tokens[1];
		}
		else if(tokens[0].compare("--pm-declare-input") == 0){
			TokenizeTrimString(tokens[1], paramTokens, '|');
			ScriptParameter param;
			if(paramTokens.size() >= 3){
				param._var_name = paramTokens[0];
				param._arg_name = paramTokens[1];
				param._type_name = ToLower(paramTokens[2]);
				if(paramTokens.size() > 3)
					param._options = paramTokens[3];
			}
			decls._inputs.push_back(param);
		}
	}

//	parse options
	std::vector<string> options;
	for(size_t iinput = 0; iinput < decls._inputs.size(); ++iinput)
	{
		ScriptParameter& param = decls._inputs[iinput];

		options.clear();
		if(!param._options.empty()){
			TokenizeTrimString(param._options, options, ';');
		}
		if((param._type_name == "double") || (param._type_name == "float")){
			param._val = 0;
			param._min = -numeric_limits<double>::max();
			param._max = numeric_limits<double>::max();
			param._step = 1;
			param._digits = 9;
			if(!options.empty()){
				for(size_t iopt = 0; iopt < options.size(); ++iopt){
					TokenizeTrimString(options[iopt], tokens, '=');
					if(tokens.size() == 2){
						if(tokens[0] == "min")
							param._min = ToNumber<double>(tokens[1]);
						else if(tokens[0] == "max")
							param._max = ToNumber<double>(tokens[1]);
						else if(tokens[0] == "val")
							param._val = ToNumber<double>(tokens[1]);
						else if(tokens[0] == "step")
							param._step = ToNumber<double>(tokens[1]);
						else if(tokens[0] == "digits")
							param._digits = ToNumber<double>(tokens[1]);
					}
					else{
						UG_LOG("Invalid option '" << options[iopt] << "' in paramter '"
							   << param._arg_name << std::endl);
					}
				}
			}
		}

		else if((param._type_name == "int") || (param._type_name == "integer")){
			param._val = 0;
			param._min = -numeric_limits<int>::max();
			param._max = numeric_limits<int>::max();
			param._step = 1;
			if(!options.empty()){
				for(size_t iopt = 0; iopt < options.size(); ++iopt){
					TokenizeTrimString(options[iopt], tokens, '=');
					if(tokens.size() == 2){
						if(tokens[0] == "min")
							param._min = ToNumber<int>(tokens[1]);
						else if(tokens[0] == "max")
							param._max = ToNumber<int>(tokens[1]);
						else if(tokens[0] == "val")
							param._val = ToNumber<int>(tokens[1]);
						else if(tokens[0] == "step")
							param._step = ToNumber<int>(tokens[1]);
					}
					else{
						UG_LOG("Invalid option '" << options[iopt] << "' in paramter '"
							   << param._arg_name << std::endl);
					}
				}
			}
		}

		else if((param._type_name == "bool") || (param._type_name == "boolean")){
			param._val = false;
			if(!options.empty()){
				for(size_t iopt = 0; iopt < options.size(); ++iopt){
					TokenizeTrimString(options[iopt], tokens, '=');
					if(tokens.size() == 2){
						if(tokens[0] == "val"){
							string tmp = ToLower(tokens[1]);
							if((tmp == "true") || (tmp == "1"))
								param._val = true;
						}
					}
					else{
						UG_LOG("Invalid option '" << options[iopt] << "' in paramter '"
							   << param._arg_name << std::endl);
					}
				}
			}
		}

		else if(param._type_name == "string"){
			param._val = "";
			if(!options.empty()){
				for(size_t iopt = 0; iopt < options.size(); ++iopt){
					TokenizeTrimString(options[iopt], tokens, '=');
					if(tokens.size() == 2){
						if(tokens[0] == "val"){
							param._val = tokens[1];
						}
					}
					else{
						UG_LOG("Invalid option '" << options[iopt] << "' in paramter '"
							   << param._arg_name << std::endl);
					}
				}
			}
		}
	}
}

void ExecuteScript (const char* scriptContent, ug::promesh::Mesh* mesh)
{

	SPLuaShell luaShell = GetDefaultLuaShell();
		
	SetScriptDefaultVariables (luaShell, scriptContent);

	luaShell->set(	"mesh", mesh, "Mesh");

	luaShell->run(scriptContent);
}

void ExecuteScriptFromFile (const char* filename, ug::promesh::Mesh* mesh)
{
	QString content = GetFileContent(filename);
	ExecuteScript (content.toLocal8Bit().constData(), mesh);
}
