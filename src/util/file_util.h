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

#ifndef __H__PROMESH_file_util
#define __H__PROMESH_file_util

#include <QString>

///	returns the content of a file as a string.
/**	To read from a resource-file, use the following syntax:
 * \code
 * GetFileContent(":/some-file.txt")
 *\endcode*/
QString GetFileContent(QString filename);

/// writes the specified content to a file
void SetFileContent(const QString& filename, const QString& content);

bool FileExists(const QString& dirname);

bool EraseDirectory(QString dirName);

///	recursively copies a directory
void CopyDirectory(QString dirName, QString destDirName);

#endif	//__H__PROMESH_file_util
