/*
 * Copyright (c) 2008-2015:  G-CSC, Goethe University Frankfurt
 * Copyright (c) 2006-2008:  Steinbeis Forschungszentrum (STZ Ölbronn)
 * Copyright (c) 2006-2015:  Sebastian Reiter
 * Author: Sebastian Reiter, modified by Markus Knodel
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

#include <QtWidgets>
#include <queue>
#include "app.hpp"
#include "standard_tools.hpp"
#include "tool_frac_to_layer.hpp"
#include "tool_frac_to_layer_arte.hpp"
#include "tooltips.h"

#include "lib_grid/algorithms/extrusion/expand_layers_arte.h"
#include "lib_grid/algorithms/extrusion/expand_layers_arte3D.h"


using namespace std;
using namespace ug;

class ToolExpandLayers2d : public ITool {
public:
	void execute(LGObject* obj, QWidget* widget) override {
		using namespace ug;

		auto dlg = dynamic_cast<FracToLayerWidget*>(widget);

		if(dlg->numEntries() == 0){
			UG_LOG("No entries selected. Aborting 'Expand Layers 2d'.\n");
			return;
		}

		Grid& grid = obj->grid();
		SubsetHandler& sh = obj->subset_handler();

		ExpandFractures2d(grid, sh, dlg->entries(), dlg->degenerated_fractures(),
						  dlg->expand_outer_boundaries());

	//	done
		obj->geometry_changed();
	}

	const char* get_name() override {return "Expand Layers 2d";}
	const char* get_tooltip() override {return TOOLTIP_EXPAND_LAYERS_2D;}
	const char* get_group() override {return "Remeshing | Layers";}

	QWidget* get_dialog(QWidget* parent) override {
		return new FracToLayerWidget(get_name(), parent, this);
	}
};

////////////////////////////////////////////////////////////////////


class ToolExpandLayers2dArte : public ITool {
public:
	void execute(LGObject* obj, QWidget* widget) override {
		using namespace ug;

//		UG_LOG("Test execute" << std::endl);

		auto* dlg = dynamic_cast<FracToLayerWidgetArte*>(widget);

		if(dlg->numEntries() == 0){
			UG_LOG("No entries selected. Aborting 'Expand Layers 2d'.\n");
			return;
		}

		Grid& grid = obj->grid();
		SubsetHandler& sh = obj->subset_handler();

//		UG_LOG("Expand" << std::endl);

		ExpandFractures2dArte(grid, sh, dlg->entries(), dlg->diamondsUseTriangles(),
						  dlg->establishDiamonds());

	//	done
		obj->geometry_changed();
	}

	const char* get_name() override {return "Expand Layers 2d Arte";}
	const char* get_tooltip() override {return TOOLTIP_EXPAND_LAYERS_2D_ARTE;}
	const char* get_group() override {return "Remeshing | Layers";}

	QWidget* get_dialog(QWidget* parent) override {
//		UG_LOG("Get dialog" << std::endl);
		return new FracToLayerWidgetArte(get_name(), parent, this);
//		return new FracToLayerWidget(get_name(), parent, this);
	}
};

////////////////////////////////////////////////////////////////////

class ToolExpandLayers3dArte : public ITool {
public:
	void execute(LGObject* obj, QWidget* widget) override {
		using namespace ug;

//		UG_LOG("Test execute" << std::endl);

		auto* dlg = dynamic_cast<FracToLayerWidgetArte*>(widget);

		if(dlg->numEntries() == 0){
			UG_LOG("No entries selected. Aborting 'Expand Layers 2d'.\n");
			return;
		}

		Grid& grid = obj->grid();
		SubsetHandler& sh = obj->subset_handler();

//		UG_LOG("Expand" << std::endl);

		ExpandFractures3dArte(grid, sh, dlg->entries(), dlg->diamondsUseTriangles(),
						  dlg->establishDiamonds());

	//	done
		obj->geometry_changed();
	}

	const char* get_name() override {return "Expand Layers 3d Arte";}
	const char* get_tooltip() override {return TOOLTIP_EXPAND_LAYERS_3D_ARTE;}
	const char* get_group() override {return "Remeshing | Layers";}

	QWidget* get_dialog(QWidget* parent) override {
//		UG_LOG("Get dialog" << std::endl);
		return new FracToLayerWidgetArte(get_name(), parent, this);
//		return new FracToLayerWidget(get_name(), parent, this);
	}
};



////////////////////////////////////////////////////////////////////

class ToolExpandLayers3d : public ITool {
public:
	void execute(LGObject* obj, QWidget* widget) override {
		using namespace ug;

		auto* dlg = dynamic_cast<FracToLayerWidget*>(widget);

		if(dlg->numEntries() == 0){
			UG_LOG("No entries selected. Aborting 'Expand Layers 3d'.\n");
			return;
		}

		Grid& grid = obj->grid();
		SubsetHandler& sh = obj->subset_handler();

		//obj->selector().clear();
		ExpandFractures3d(grid, sh, dlg->entries(), dlg->degenerated_fractures(),
						dlg->expand_outer_boundaries()/*, obj->selector()*/);

	//	done
		obj->geometry_changed();
	}

	const char* get_name() override {return "Expand Layers 3d";}
	const char* get_tooltip() override {return TOOLTIP_EXPAND_LAYERS_3D;}
	const char* get_group() override {return "Remeshing | Layers";}

	QWidget* get_dialog(QWidget* parent) override {
		return new FracToLayerWidget(get_name(), parent, this);
	}
};


void RegisterFracToLayerTools(ToolManager* toolMgr)
{
    toolMgr->register_tool(new ToolExpandLayers2d);
    toolMgr->register_tool(new ToolExpandLayers3d);
    toolMgr->register_tool(new ToolExpandLayers2dArte);
    toolMgr->register_tool(new ToolExpandLayers3dArte);


}

