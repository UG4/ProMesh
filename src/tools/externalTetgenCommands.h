/*
 * Copyright (c) 2017:  G-CSC, Goethe University Frankfurt, 2025 TechSim
 * Author: Markus Knodel
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

#ifndef __H__PROMESH_externalTetgenCommands__
#define __H__PROMESH_externalTetgenCommands__

#include <QString>
#include <QProcess>
#include "promesh_plugin.h"
#include "app.h"
#include "standard_tools.h"
#include "registry/registry.h"
#include "bridge/util.h"
#include "tooltips.h"
#include "tools/file_io_tools.h"
#include "lib_grid/file_io/file_io_tetgen.h"
#include <vector>
#include <fstream>
#include "app.h"
#include "standard_tools.h"
#include "tools_util.h"
#include "lib_grid/algorithms/remeshing/delaunay_triangulation.h"
#include "tools/grid_generation_tools.h"
#include "tooltips.h"
#include "../scene/csg_object.h"
#include "script_tools.h"
#include "lib_grid/file_io/file_io_vtu.h"

//using namespace ug;
//using namespace std;
//using namespace ug::promesh;
//using namespace ug::bridge;
//using namespace app;


namespace externalCommands
{

extern QString globVarTetgenCall;

};



#endif	//__H__PROMESH_externalTetgenCommands__
