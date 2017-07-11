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

#ifndef __H__PROMESH_tetgen_tools
#define __H__PROMESH_tetgen_tools

#include <QString>
#include <QProcess>
#include "promesh_plugin.h"
#include "app.h"
#include "standard_tools.h"
#include "registry/registry.h"
#include "bridge/util.h"
#include "tooltips.h"
#include "tools/file_io_tools.h"

using namespace ug;
using namespace std;
using namespace ug::promesh;
using namespace ug::bridge;
using namespace app;


static QString BuildTetgenArguments (
        bool hasFaces,
		number quality,
		bool preserveOuter,
		bool preserveAll,
		int verbosity)
{
	QString args;
	if(verbosity == 1)
		args.append("V");
	else if(verbosity == 2)
		args.append("VV");
	else if(verbosity >= 3)
		args.append("VVV");

	if(hasFaces){
		args.append("p");
		if(quality > SMALL)
			args.append("qq").append(QString::number(quality));
		if(preserveOuter || preserveAll)
			args.append("Y");
		if(preserveAll)
			args.append("Y");	// if inner bnds shall be preserved "YY" has to be passed to tetgen
	}

	args.append("Q");
	return args;
}

void TetrahedralizeEx (	Mesh* mesh,
                       	number quality,
						bool preserveOuter,
						bool preserveAll,
						bool separateVolumes,
						bool appendSubsetsAtEnd,
						int verbosity)
{

	QString outFileName = TmpFileName("plc", ".smesh");
	// UG_LOG("Saving to file: " << outFileName.toLocal8Bit().constData() << std::endl);

	if(!SaveMesh(mesh, outFileName.toLocal8Bit().constData())){
		UG_THROW("SaveMesh failed with mesh '" << outFileName.toLocal8Bit().constData() << "' in Tetrahedral Fill\n");
	}

	QString args;
	args.append("-").append(BuildTetgenArguments(mesh->grid().num_faces() > 0,
                                quality,
                                preserveOuter,
                                preserveAll,
                                verbosity));
	args.append(" ").append(outFileName);


	QString call = AppDir().path()	.append(QDir::separator())
									.append("tools")
									.append(QDir::separator())
									.append("tetgen");
	// UG_LOG("Calling: '" << call.toLocal8Bit().constData() << "'\n");

	UG_LOG("Calling 'tetgen' by Hang Si (www.tetgen.org)\n");

	QProcess proc;
	proc.setProcessChannelMode(QProcess::MergedChannels);
	proc.start(call, args.split(' '));

	if(!proc.waitForFinished(-1)){
		UG_LOG(proc.errorString().toLocal8Bit().constData() << "\n");
	}
	else{
		QString output(proc.readAll());
		output.replace(QString("\r\n"), QString("\n"));
		output.replace(QString("\r"), QString("\n"));
		UG_LOG(output.toLocal8Bit().constData() << "\n");
	}

	mesh->grid().clear_geometry();
	QString inFileName(outFileName);
	inFileName.replace(QString(".smesh"), QString(".1.ele"));
	LoadMesh(mesh, inFileName.toLocal8Bit().constData());

//	remove temporary files
	QFile::remove(outFileName);
	QFile::remove(inFileName);
	inFileName.replace(QString(".1.ele"), QString(".1.node"));
	QFile::remove(inFileName);
	inFileName.replace(QString(".1.node"), QString(".1.edge"));
	QFile::remove(inFileName);
	inFileName.replace(QString(".1.edge"), QString(".1.face"));
	QFile::remove(inFileName);

	SubsetHandler& sh = mesh->subset_handler();
	Grid& grid = mesh->grid();

	int oldNumSubsets = sh.num_subsets();
	if(separateVolumes){
		SeparateSubsetsByLowerDimSubsets<Volume>(grid, sh, appendSubsetsAtEnd);
	}
	else if(appendSubsetsAtEnd){
		sh.assign_subset(grid.begin<Tetrahedron>(),
						 grid.end<Tetrahedron>(), sh.num_subsets());
	}

//	assign a subset name
	for(int i = oldNumSubsets; i < sh.num_subsets(); ++i)
		sh.subset_info(i).name = "tetrahedra";

	UG_LOG("Done\n");
}


void RegisterTetgenTools ()
{
	ProMeshRegistry& reg = GetProMeshRegistry();

	string grp = "Remeshing/Tetrahedra";
	reg.add_function("TetrahedralFill", &TetrahedralizeEx, grp, "",
				"mesh #"
				"quality || value=5; min=0; max=18; step=1 #"
				"preserve outer #"
				"preserve all #"
				"separate volumes || value=true #"
				"append subsets at end || value=true#"
				"verbosity || min=0; value=0; max=3; step=1",
				"Fills a closed surface with tetrahedra using TetGen.");
}

#endif	//__H__UG_tetgen_tools
