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

#ifndef UNDO_H
#define UNDO_H

#include <string>
#include <deque>
#include <stack>
#include <qdir.h>

class UndoHistory
{
	public:
		UndoHistory();
		UndoHistory(const char* fileNamePrefix, int maxSteps);

	/** returns true if undo is possible for the given id*/
		bool can_undo();

	/** returns true if redo is possible for the given id*/
		bool can_redo();

	/** returns the name of the last valid undo-file and nullptr if
	 *	none exists. The undo-file is popped from the stack
	 *	The returned pointer is valid until the next call of
	 *	either undo, redo or create_history_entry.*/
		const char* undo();

	/** returns the name of the next valid redo-file and nullptr if
	 *	none exists. The redo-file is popped from the stack
	 *	The returned pointer is valid until the next call of
	 *	either undo, redo or create_history_entry.*/
		const char* redo();

	/**	returns the filename for the next history entry.
	 *	Please note that this action clears the redo-stack.
	 *	The returned pointer is valid until the next call of
	 *	either undo, redo or create_history_entry.*/
		const char* create_history_entry();

	/**	Sets a suffix which is appended to each filename.*/
		void set_suffix(const char* suffix);

	private:
		using FileQueue = std::deque<std::string>;
		using FileStack = std::stack<std::string>;

		bool _initialized;
		int _counter;
		std::string _prefix;
		std::string _suffix;
		std::string _current_file;
		FileQueue _undo_files;
		FileStack _redo_files;
		int _max_steps;
		int _num_steps;

};

class UndoHistoryProvider
{
	public:
		static UndoHistoryProvider& inst();

	/** creates the folder path/.history.*/
		bool init(const char* path);

	/** returns a unique id with which a undo-history can
	 *	be associated.*/
		UndoHistory create_undo_history();

	private:
		UndoHistoryProvider();
		~UndoHistoryProvider();

	private:
		int _max_undo_steps;
		std::string	_path;				//the complete path
		std::string	_history_dir_name;	//only the name of the history directory
		QDir _parent_dir;
		int _history_counter;
};

#endif