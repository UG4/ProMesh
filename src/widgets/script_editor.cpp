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

#include <QFileDialog>
#include <QLabel>
#include <QMenuBar>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include "script_editor.hpp"
#include "../app.hpp"
#include "../scripting.hpp"
#include "../util/file_util.hpp"
#include "../util/qstring_util.hpp"
#include "../widgets/truncated_double_spin_box.hpp"

using namespace std;

QScriptEditor::QScriptEditor (QWidget* parent) :
	QDialog (parent),
	_filename (tr("untitled.psc")),
	_replay_target (nullptr)
{

	this->setWindowTitle("Script Editor");
	this->setObjectName("ScriptEditor");

	auto* layout = new QVBoxLayout(this);

//	menu
	auto actOpen = new QAction (tr("Open"), this);
	actOpen->setIcon(QIcon(":images/fileopen.png"));
	actOpen->setToolTip(tr("Opens an existing script from a file"));

	auto* actSave = new QAction (tr("Save"), this);
	actSave->setIcon(QIcon(":images/filesave.png"));
	actSave->setToolTip(tr("Saves the current script to a file"));

	connect(actOpen, &QAction::triggered, this, &QScriptEditor::openFile);
	connect(actSave, &QAction::triggered, this, &QScriptEditor::saveToFile);

	auto* menuBar = new QMenuBar (this);
	QMenu* menu = menuBar->addMenu("&File");
	menu->addAction(actOpen);
	menu->addAction(actSave);

	layout->setMenuBar (menuBar);

//	widgets
	layout->setSpacing(5);
	layout->setContentsMargins(0, 0, 0, 0);

	QFont editorFont("unknown");
	editorFont.setStyleHint(QFont::Monospace);
	editorFont.setPointSize(10);

	_text_edit = new QPlainTextEdit (this);
	_text_edit->setLineWrapMode (QPlainTextEdit::NoWrap);
	_text_edit->setFont(editorFont);
	_text_edit->setTabStopDistance(40);
	layout->addWidget (_text_edit);

	auto* hlayout = new QHBoxLayout();
	hlayout->setContentsMargins(4, 4, 4, 4);

	auto lblReplayInterval = new QLabel (tr("replay interval:"), this);
	lblReplayInterval->setContentsMargins(2, 2, 2, 2);
	hlayout->addWidget(lblReplayInterval, 0, Qt::AlignLeft);

	_interval_widget = new TruncatedDoubleSpinBox (this);
	_interval_widget->setRange(0, 1.e+12);
	_interval_widget->setDecimals(3);
	_interval_widget->setLocale(QLocale(tr("C")));
	_interval_widget->setSingleStep(0.05);
	_interval_widget->setValue(0);
	hlayout->addWidget(_interval_widget, 0, Qt::AlignLeft);

	auto* lblSec = new QLabel (tr("sec"), this);
	lblSec->setContentsMargins(2, 2, 2, 2);
	hlayout->addWidget(lblSec, 0, Qt::AlignLeft);

	hlayout->addStretch (1.f);

	_apply_button = new QPushButton(tr("Apply"), this);
	hlayout->addWidget(_apply_button, 0, Qt::AlignRight);
	connect(_apply_button, &QPushButton::clicked, this, &QScriptEditor::apply);

	layout->addLayout(hlayout);

	QSettings settings;
	resize(settings.value("scriptEditor/size", QSize(600, 768)).toSize());
	move(settings.value("scriptEditor/pos", QPoint(10, 10)).toPoint());

	_timer = new QTimer(this);
	connect(_timer, &QTimer::timeout, this, &QScriptEditor::timeout);
}

void QScriptEditor::resizeEvent (QResizeEvent* evt)
{
	QSettings().setValue("scriptEditor/size", size());
}

void QScriptEditor::moveEvent(QMoveEvent *event)
{
	QSettings().setValue("scriptEditor/pos", pos());
}

void QScriptEditor::openFile()
{
	QString path = QSettings().value("scriptEditor/filePath", ".").toString();

	QString filename = QFileDialog::getOpenFileName(
								this,
								tr("Load Script"),
								path,
								tr("script files (*.psc)"));

	if(!filename.isEmpty()){
		QSettings().setValue("scriptEditor/filePath", QFileInfo(filename).absolutePath());
		_text_edit->setPlainText(GetFileContent(filename));
		_filename = filename;
	}
}


void QScriptEditor::saveToFile()
{
	QString path = QSettings().value("scriptEditor/filePath", ".").toString();
	path.append("/").append(_filename);

	QString filename = QFileDialog::getSaveFileName(
								this,
								tr("Save Script"),
								path,
								tr("script files (*.psc)"));

	if(!filename.isEmpty()){
		_filename = filename;
		QSettings().setValue("scriptEditor/filePath", QFileInfo(filename).absolutePath());
		SetFileContent(filename, _text_edit->toPlainText());
	}
}

void QScriptEditor::apply ()
{
	if(_timer->isActive()){
	//	We're currently in playback. Stop playback.
		stopReplay();
		return;
	}

	LGObject* obj = app::getActiveObject();
	if(!obj){
	//todo: create the appropriate object for the current module
		obj = app::createEmptyObject("new mesh", SOT_LG);
	}

	try{
		SPLuaShell luaShell = GetDefaultLuaShell();
		
		QByteArray scriptContent = To7BitAscii(_text_edit->toPlainText());
		// QByteArray scriptContent = m_textEdit->toPlainText().toLocal8Bit();

		SetScriptDefaultVariables (luaShell, scriptContent.constData());

		luaShell->set(	"mesh", obj, "Mesh");

		const double ri = _interval_widget->value();
		if(ri == 0)
			luaShell->run(scriptContent.constData());
		else{
			_replay_target = obj;
			_replay_stream.str(scriptContent.constData());
			_apply_button->setText(tr("Cancel"));
			_timer->setInterval (int (ri * 1000));
			_timer->start ();

		}
	}
	catch(ug::script::LuaError& err) {
		ug::PathProvider::clear_current_path_stack();
		if(err.show_msg()){
			if(!err.get_msg().empty()){
				UG_LOG("ERROR in live script:\n");
				for(size_t i=0;i<err.num_msg();++i)
					UG_LOG(err.get_msg(i)<<std::endl);
			}
		}
	}

	obj->geometry_changed();
}


void QScriptEditor::timeout()
{
//	make sure that the target object is valid and still contained in the scene
	if(!_replay_target){
		stopReplay();
		return;
	}

	LGScene* scn = app::getActiveScene();
	UG_COND_THROW(!scn, "There has to be a scene object!");

	bool foundIt = false;
	for(int i = 0; i < scn->num_objects(); ++i){
		if(scn->get_object(i) == _replay_target){
			foundIt = true;
			break;
		}
	}

	if(!foundIt){
		stopReplay();
		return;
	}


//	find the next line which contains a command
//todo:	support multi-line commands
	string line;
	while(line.empty() && !_replay_stream.eof()) {
		getline(_replay_stream, line);
	//	remove comments and white spaces
		size_t p = line.find("--");
		if(p != string::npos)
			line.resize(p);

		while((line.size() > 0) && (line[line.size()-1] == ' '))
			line.resize(line.size() - 1);
	}

	if(line.empty()){
	//	replay is done
		stopReplay();
	}
	else{
		SPLuaShell luaShell = GetDefaultLuaShell();
		try{
			UG_LOG("Replay: " << line << endl);
		//	execute the command
			luaShell->run(line.c_str());
			_replay_target->geometry_changed();
		}

		catch(ug::script::LuaError& err) {
			ug::PathProvider::clear_current_path_stack();
			if(err.show_msg()){
				if(!err.get_msg().empty()){
					UG_LOG("ERROR during script replay:\n");
					for(size_t i=0;i<err.num_msg();++i)
						UG_LOG(err.get_msg(i)<<std::endl);
				}
			}

			_replay_target->geometry_changed();
			stopReplay();
		}
	}
}

void QScriptEditor::stopReplay()
{
	_timer->stop();
	_replay_target = nullptr;
	_apply_button->setText(tr("Apply"));
}
