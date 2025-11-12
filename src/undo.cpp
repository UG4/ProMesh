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

#include <string>
#include <sstream>
#include "undo.hpp"
#include "common/math/misc/math_util.h"
#include "common/util/file_util.h"
#include "common/util/string_util.h"

using namespace std;
using namespace ug;

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//	UndoHistory implementation
UndoHistory::
UndoHistory() :
	_initialized(false)
{
}

UndoHistory::
UndoHistory(const char* fileNamePrefix, int maxSteps) :
	_initialized(true),
	_counter(0),
	_prefix(fileNamePrefix),
	_max_steps(maxSteps),
	_num_steps(0)
{
}

bool UndoHistory::
can_undo()
{
	if(!_initialized)
		return false;
	return !_undo_files.empty();
}

bool UndoHistory::
can_redo()
{
	if(!_initialized)
		return false;
	return !_redo_files.empty();
}

const char* UndoHistory::
undo()
{
	if(!_initialized)
		return nullptr;

	if(_undo_files.empty())
		return nullptr;

//	push the current file to the redo stack
	if(!_current_file.empty())
		_redo_files.push(_current_file);
	_current_file = _undo_files.back();
	_undo_files.pop_back();
	--_num_steps;

	return _current_file.c_str();
}

const char* UndoHistory::
redo()
{
	if(!_initialized)
		return nullptr;

	if(_redo_files.empty())
		return nullptr;

//	push the current file to the back of the undo files.
	if(!_current_file.empty())
		_undo_files.push_back(_current_file);
	_current_file = _redo_files.top();
	_redo_files.pop();
	++_num_steps;

//	we don't have to check for too many undo-files here,
//	since only existing files are restored.

	return _current_file.c_str();
}

const char* UndoHistory::
create_history_entry()
{
	if(!_initialized)
		return nullptr;

//	clear the redo stack
	if(!_redo_files.empty()){
		while(!_redo_files.empty()){
			QFile rmFile(_redo_files.top().c_str());
			_redo_files.pop();
			rmFile.remove();
		}
	}

//	add undo entry
	if(!_current_file.empty()){
		_undo_files.push_back(_current_file);
		++_num_steps;
	}

//	set up the new file
	stringstream ss;
	ss << _prefix << _counter++ << _suffix;
	_current_file = ss.str();

//	check whether we have to erase a file
	if(_num_steps == _max_steps){
		--_num_steps;
		QFile rmFile(_undo_files.front().c_str());
		_undo_files.pop_front();
		rmFile.remove();
	}

	return _current_file.c_str();
}

void UndoHistory::
set_suffix(const char *suffix)
{
	_suffix = suffix;
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//	UndoHistoryProvider implementation
UndoHistoryProvider::
UndoHistoryProvider() :
	_max_undo_steps(100),
	_history_counter(0)
{
}

UndoHistoryProvider::
~UndoHistoryProvider()
{
	if(!_path.empty()){
	//	remove all files in .history
		QDir history(_parent_dir);
		if(history.cd(_history_dir_name.c_str())){
			QStringList fileNames = history.entryList();
			for(auto iter = fileNames.begin(); iter != fileNames.end(); ++iter)
			{
				history.remove(*iter);
			}
		}

	//	remove history itself
		_parent_dir.rmdir(_history_dir_name.c_str());
	}
}

UndoHistoryProvider& UndoHistoryProvider::
inst()
{
	static UndoHistoryProvider ufp;
	return ufp;
}

bool UndoHistoryProvider::
init(const char* path)
{
	if(_path.empty()){
		_path.append(path).append("/");
	//	append a unique number to the path so that each promesh instance
	//	has its own history path
		bool gotOne = false;
		for(int i = 0; i < 1000; ++i){
			_history_dir_name = ".history";
			_history_dir_name.append(ToString(urand<int>(100000, 999999)));
			string tpath = _path;
			tpath.append(_history_dir_name);
			if(!DirectoryExists(tpath)){
				gotOne = true;
				break;
			}
		}

		if(!gotOne)
			return false;

		_path.append(_history_dir_name);
		_parent_dir.setPath(path);
		return _parent_dir.mkdir(_history_dir_name.c_str());
	}

	return false;
}

UndoHistory UndoHistoryProvider::
create_undo_history()
{
	stringstream ss;
	ss << _path << "/entry_" << _history_counter << "_";
	++_history_counter;
	return UndoHistory(ss.str().c_str(), _max_undo_steps);
}
