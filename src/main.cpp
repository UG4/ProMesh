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

#include <QApplication>
#include <QSurfaceFormat>
#include <iostream>
#include <clocale>
#include <cstring>
#include <QFileOpenEvent>
#include "app.hpp"
#include "arg_tool.hpp"
#include "docugen.hpp"
#include "scripting.hpp"
#include "tools/standard_tools.hpp"
#include "bridge/bridge.h"
#include "common/util/path_provider.h"
#include "common/util/plugin_util.h"
#include "options/options.hpp"
#include "util/file_util.hpp"
//TESTING
#include <QDialog>
#include <QVBoxLayout>

using namespace std;

class MyApplication : public QApplication {
	public:
		MyApplication(int & argc, char ** argv) :
			QApplication(argc, argv), m_pMainWindow(nullptr)	{}

		bool event ( QEvent * e ) override {
			if(e->type() == QEvent::FileOpen){
				auto* foe = dynamic_cast<QFileOpenEvent*>(e);
				if(foe){
					QString str = foe->file();
					if(m_pMainWindow){
						if(m_pMainWindow->load_grid_from_file(str.toLocal8Bit().constData()))
						{
							m_pMainWindow->settings().setValue("file-path",
												QFileInfo(str).absolutePath());
						}
					}
					return true;
				}
			}
			return QApplication::event(e);
		}

		void setMainWindow(MainWindow* win){
			m_pMainWindow = win;
		}

	private:
		MainWindow* m_pMainWindow;
};


static void WriteToFileInUserDataDir(const char* filename, const QString& content)
{
	SetFileContent (app::UserDataDir().absoluteFilePath(filename),
	                content);
}


int main(int argc, char *argv[])
{
	QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

	MyApplication myApp(argc, argv);
	MyApplication::setQuitOnLastWindowClosed(true);
	//MyApplication::setAttribute (Qt::AA_UseDesktopOpenGL);
	
	QCoreApplication::setOrganizationName("ProMesh");
    QCoreApplication::setOrganizationDomain("promesh3d.com");
    QCoreApplication::setApplicationName("ProMesh");

    {
    //	write the absolute path of the application to .promesh/promesh_home
    	WriteToFileInUserDataDir ("promesh_home", app::AppDir().path());
    	WriteToFileInUserDataDir ("promesh_version", app::GetVersionString());
    }

    {
    //	no-gui script processing
	    ArgTool args(argc, (const char**) argv);
	    
		std::string scriptName = args.get_string ("-script", "",
	                                          "(filename): executes a meshing script.\n"
	                                          "No GUI will be started. The application exits\n"
	                                          "when the script is done.\n"
	                                          "You may specify in- and out-files through the\n"
	                                          "-in and -out options. Please use the -help parameter\n"
	                                          "for more information.");

		string inFile = args.get_string ("-in", "",
									"(filename): Loads a mesh for script processing.\n"
									"Only relevant if '-script ...' is specified.");

		string outFile = args.get_string ("-out", "",
									"(filename): The resulting mesh from script processing\n"
									"will be saved to this file.\n"
									"Only relevant if '-script ...' is specified.");

		if(args.has_param ("-help", "Prints help on command line usage")){
			cout << "Command line options for ProMesh.\n\n";
			cout << args.get_help() << endl;
			return 0;
		}

	    if(!scriptName.empty()){
	    	const bool undoEnabled = GetOptions()._undo._enabled;
	    	GetOptions()._undo._enabled = false;
	    	int retVal = 0;
	    	try{
	    		try{
					ug::bridge::InitBridge();
					if(!ug::LoadPlugins(ug::PathProvider::get_path(ug::PLUGIN_PATH).c_str(), "ug4/", ug::bridge::GetUGRegistry()))
					{
						UG_LOG("ERROR during initialization of plugins: LoadPlugins failed!\n");
					}
	    			RegisterTetgenTools();
				}
				catch(ug::UGError& err){
					UG_LOG("ERROR during initialization of ug::bridge:\n");
					for(size_t i = 0; i < err.num_msg(); ++i){
						UG_LOG("  " << err.get_msg(i) << std::endl);
					}
					UG_THROW("initialization failed");
				}

		    	auto* obj = new LGObject();
		    	if(!inFile.empty()){
		    		cout << "loading mesh from '" << inFile.c_str() << "'\n";
		    		UG_COND_THROW(!LoadLGObjectFromFile(obj, inFile.c_str(), false),
		    		              	"Failed to load file " << inFile);
		    	}
		    	else if(argv[1][0] != '-'){
		    		cout << "loading mesh from '" << argv[1] << "'\n";
		    		UG_COND_THROW(!LoadLGObjectFromFile(obj, argv[1], false),
		    		              	"Failed to load file " << argv[1]);
		    	}

		    	cout << "Executing script '" << scriptName << "'\n";
		    	ExecuteScriptFromFile(scriptName.c_str(), obj);
		    	
		    	if(!outFile.empty()){
		    		cout << "saving mesh to '" << outFile << "'\n";
		    		UG_COND_THROW(!SaveLGObjectToFile(obj, outFile.c_str()),
		    		              "Failed to save file " << outFile);
		    	}

		    	delete obj;
		    }
	    	catch(ug::UGError& err){
				UG_LOG("ERROR during script execution\n");
				for(size_t i = 0; i < err.num_msg(); ++i){
					UG_LOG("  " << err.get_msg(i) << std::endl);
				}
				retVal = 1;
			}
		    catch(...){
		    	cout << "An error occurred during execution\n";
		    	retVal = 1;
		    }
		    GetOptions()._undo._enabled = undoEnabled;
		    cout << "Script execution done. Finishing...\n";
	    	return retVal;
	    }
	}

    std::ostream::sync_with_stdio(true);


	QString qss = GetFileContent(":/styles/promesh_style.css");
	QString varsStr = GetFileContent(":/styles/dark_theme_variables.txt");
	// QString qss = GetFileContent("C:\\Users\\sreiter\\projects\\ProMesh\\ProMesh\\styles\\promesh_style.css");
	// QString varsStr = GetFileContent("C:\\Users\\sreiter\\projects\\ProMesh\\ProMesh\\styles\\dark_theme_variables.txt");
	// QString qss = GetFileContent("/home/sreiter/projects/ProMesh/ProMesh/styles/promesh_style.css");
	// QString varsStr = GetFileContent("/home/sreiter/projects/ProMesh/ProMesh/styles/dark_theme_variables.txt");
	QStringList varsList = varsStr.split(QRegularExpression("[\r\n]"),Qt::SkipEmptyParts);
	QRegularExpression regVar("\\s*(@\\w+)\\s*(.+)");
	QMap<QString, QString> varMap;
	for(auto iter = varsList.begin(); iter != varsList.end(); ++iter){
		QRegularExpressionMatch match = regVar.match(*iter);
		if(match.hasMatch()){
			varMap[match.captured(1)] = match.captured(2);
		}
	}
	
	QMapIterator mapIter(varMap);
	mapIter.toBack();
	while(mapIter.hasPrevious()){
		mapIter.previous();
		qss.replace(mapIter.key(), mapIter.value());
	}

	myApp.setStyleSheet(qss);

	MainWindow* pMainWindow = app::getMainWindow();

	pMainWindow->init();

	myApp.setMainWindow(pMainWindow);
	pMainWindow->setWindowTitle(QString("ProMesh").append(app::GetVersionString()).append(""));

	pMainWindow->show();

	setlocale(LC_NUMERIC, "C");

	for(int i = 1; i < argc; ++i){
		if(argv[i][0] == '-')
			break;

		if(pMainWindow->load_grid_from_file(argv[i])){
			pMainWindow->settings().setValue("file-path",
											 QFileInfo(argv[i]).absolutePath());
		}
	}

	// dlg->show();

	#ifdef UG_DEBUG
		UG_LOG("DEBUG MODE\n");
	#endif
	#ifdef UG_ENABLE_DEBUG_LOGS
		UG_LOG("DEBUG LOGS ACTIVE\n");
		UG_SET_DEBUG_LEVEL(ug::LIB_GRID, 1);
	#endif

	if((argc > 1) && (strcmp(argv[1], "-docugen") == 0)){
		UG_LOG("Executing docugen...\n");
		const int retVal = RunDocugen();
		ofstream out("docugen.log");
		out << pMainWindow->log_text() << endl;
		out.close();
		if(!retVal){
			return 0;
		}
		else{
			UG_LOG("\nAN ERROR OCCURRED DURING DOCUGEN.\n");
		}
	}

	return MyApplication::exec();
}
