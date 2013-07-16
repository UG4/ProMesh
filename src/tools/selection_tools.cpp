//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y10 m05 d07

#include <stack>
#include <vector>
#include "app.h"
#include "standard_tools.h"
#include "tools_util.h"
#include "tools/selection_tools.h"

using namespace ug;

class ToolSelectLinkedManifoldFaces : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::SelectLinkedManifoldFaces(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Linked Manifold Faces";}
		const char* get_tooltip()	{return "Selects faces linked with the selection, not crossing non-manifold edges.";}
		const char* get_group()		{return "Selection | Faces";}
};

class ToolSelectNonManifoldEdges : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::SelectNonManifoldEdges(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Non Manifold Edges";}
		const char* get_tooltip()	{return "Selects edges with more than 2 associated faces.";}
		const char* get_group()		{return "Selection | Edges";}
};

class ToolClearSelection : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::ClearSelection(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Clear Selection";}
		const char* get_tooltip()	{return "Clears the selection";}
		const char* get_group()		{return "Selection";}
};


class ToolSelectSmoothEdgePath : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);
			if(dlg)
				promesh::SelectSmoothEdgePath(obj,
											  dlg->to_double(0),
											  dlg->to_bool(1));
			else
				promesh::SelectSmoothEdgePath(obj, 15., true);
			obj->selection_changed();
		}

		const char* get_name()		{return "Smooth Path";}
		const char* get_tooltip()	{return "Selects a smooth edge path.";}
		const char* get_group()		{return "Selection | Edges";}

		ToolWidget* get_dialog(QWidget* parent){
			ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
											IDB_APPLY | IDB_OK | IDB_CLOSE);
			dlg->addSpinBox(tr("max deviation:"), 0, 180, 20, 1, 0);
			dlg->addCheckBox(tr("stop at selected vertices:"), true);
			return dlg;
		}
};

class ToolSelectBoundaryVertices : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::SelectBoundaryVertices(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Boundary Vertices";}
		const char* get_tooltip()	{return "Selects vertices that lie on the boundary of the geometry";}
		const char* get_group()		{return "Selection | Vertices";}
};

class ToolSelectInnerVertices : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::SelectInnerVertices(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Inner Vertices";}
		const char* get_tooltip()	{return "Selects vertices that do not lie on the boundary of the geometry";}
		const char* get_group()		{return "Selection | Vertices";}
};

class ToolSelectBoundaryEdges : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::SelectBoundaryEdges(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Boundary Edges";}
		const char* get_tooltip()	{return "Selects edges that lie on the boundary of the geometry";}
		const char* get_group()		{return "Selection | Edges";}
};

class ToolSelectInnerEdges : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::SelectInnerEdges(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Inner Edges";}
		const char* get_tooltip()	{return "Selects edges that do not lie on the boundary of the geometry";}
		const char* get_group()		{return "Selection | Edges";}
};

class ToolSelectBoundaryFaces : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::SelectBoundaryFaces(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Boundary Faces";}
		const char* get_tooltip()	{return "Selects faces that lie on the boundary of the geometry";}
		const char* get_group()		{return "Selection | Faces";}
};

class ToolSelectInnerFaces : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::SelectInnerFaces(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Inner Faces";}
		const char* get_tooltip()	{return "Selects faces that do not lie on the boundary of the geometry";}
		const char* get_group()		{return "Selection | Faces";}
};


class ToolSelectShortEdges : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);

			number maxLength = 0.0001;
			if(dlg){
				maxLength = dlg->to_double(0);
			}

			size_t numSel = obj->get_selector().num<EdgeBase>();
			promesh::SelectShortEdges(obj, maxLength);
			UG_LOG(obj->get_selector().num<EdgeBase>() - numSel
				   << " short edges selected.\n");

			obj->selection_changed();
		}

		const char* get_name()		{return "Short Edges";}
		const char* get_tooltip()	{return "Selects edges that are shorter than a given threshold.";}
		const char* get_group()		{return "Selection | Edges";}

		ToolWidget* get_dialog(QWidget* parent){
			ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
											IDB_APPLY | IDB_OK | IDB_CLOSE);
			dlg->addSpinBox(tr("max length:"), 0, 1000000000., 0.001, 0.0001, 9);
			return dlg;
		}
};

class ToolSelectLongEdges : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);

			number minLength = 1;
			if(dlg){
				minLength = dlg->to_double(0);
			}

			size_t numSel = obj->get_selector().num<EdgeBase>();
			promesh::SelectLongEdges(obj, minLength);
			UG_LOG(obj->get_selector().num<EdgeBase>() - numSel
				   << " long edges selected.\n");


			obj->selection_changed();
		}

		const char* get_name()		{return "Long Edges";}
		const char* get_tooltip()	{return "Selects edges that are longer than a given threshold.";}
		const char* get_group()		{return "Selection | Edges";}

		ToolWidget* get_dialog(QWidget* parent){
			ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
											IDB_APPLY | IDB_OK | IDB_CLOSE);
			dlg->addSpinBox(tr("min length:"), 0, 1.e12, 1, 0.1, 9);
			return dlg;
		}
};

class ToolSelectDegenerateFaces : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);

			number maxHeight = 0.0001;
			if(dlg){
				maxHeight = dlg->to_double(0);
			}

			size_t numSelFaces = obj->get_selector().num<Face>();
			promesh::SelectDegenerateFaces(obj, maxHeight);
			UG_LOG(obj->get_selector().num<Face>() - numSelFaces
				   << " degenerated faces selected.\n");

			obj->selection_changed();
		}

		const char* get_name()		{return "Degenerate Faces";}
		const char* get_tooltip()	{return "Selects faces that have a height shorter than a given threshold.";}
		const char* get_group()		{return "Selection | Faces";}

		ToolWidget* get_dialog(QWidget* parent){
			ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
											IDB_APPLY | IDB_OK | IDB_CLOSE);
			dlg->addSpinBox(tr("max height:"), 0, 1000000000., 0.001, 0.0001, 9);
			return dlg;
		}
};

class ToolSelectLinkedFlatFaces : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);

			number maxDeviationAngle = 1;
			bool traverseFlipped = false;
			bool traverseDegeneratedFaces = false;
			bool stopAtSelectedEdges = false;

			if(dlg){
				maxDeviationAngle = dlg->to_double(0);
				traverseFlipped = dlg->to_bool(1);
				traverseDegeneratedFaces = dlg->to_bool(2);
				stopAtSelectedEdges = dlg->to_bool(3);
			}

			promesh::SelectLinkedFlatFaces(obj, maxDeviationAngle, traverseFlipped,
										   traverseDegeneratedFaces, stopAtSelectedEdges);

			obj->selection_changed();
		}

		const char* get_name()		{return "Linked Flat Faces";}
		const char* get_tooltip()	{return "Selects linked faces of selected faces that have a similar normal.";}
		const char* get_group()		{return "Selection | Faces";}

		ToolWidget* get_dialog(QWidget* parent){
			ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
											IDB_APPLY | IDB_OK | IDB_CLOSE);
			dlg->addSpinBox(tr("max deviation angle:"), 0, 180, 1, 1, 6);
			dlg->addCheckBox(tr("traverse flipped faces:"), false);
			dlg->addCheckBox(tr("traverse degenerated faces:"), false);
			dlg->addCheckBox(tr("stop at selected edges:"), false);

			return dlg;
		}
};


class ToolSelectLinkedBoundaryEdges : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);

			bool stopAtSelectedVrts = true;
			if(dlg){
				stopAtSelectedVrts = dlg->to_bool(0);
			}

			promesh::SelectLinkedBoundaryEdges(obj, stopAtSelectedVrts);

			obj->selection_changed();
		}

		const char* get_name()		{return "Linked Boundary Edges";}
		const char* get_tooltip()	{return "Selects linked boundary edges of selected edges.";}
		const char* get_group()		{return "Selection | Edges";}

		ToolWidget* get_dialog(QWidget* parent){
			ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
											IDB_APPLY | IDB_OK | IDB_CLOSE);
			dlg->addCheckBox(tr("stop at selected vertices:"), true);

			return dlg;
		}
};

class ToolSelectLinkedBoundaryFaces : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);

			bool stopAtSelectedEdges = true;

			if(dlg){
				stopAtSelectedEdges = dlg->to_bool(0);
			}

			SelectLinkedBoundaryFaces(obj, stopAtSelectedEdges);

			obj->selection_changed();
		}

		const char* get_name()		{return "Linked Boundary Faces";}
		const char* get_tooltip()	{return "Selects linked boundary faces of selected faces.";}
		const char* get_group()		{return "Selection | Faces";}

		ToolWidget* get_dialog(QWidget* parent){
			ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
											IDB_APPLY | IDB_OK | IDB_CLOSE);
			dlg->addCheckBox(tr("stop at selected edges:"), true);

			return dlg;
		}
};

class ToolSelectIntersectingTriangles : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){

			size_t numSelFaces = obj->get_selector().num<Face>();
			promesh::SelectIntersectingTriangles(obj);
			UG_LOG(obj->get_selector().num<Face>() - numSelFaces
				   << " intersecting faces selected.\n");

			obj->selection_changed();
		}

		const char* get_name()		{return "Intersecting Triangles";}
		const char* get_tooltip()	{return "Selects intersecting triangles. Neighbors are ignored.";}
		const char* get_group()		{return "Selection | Faces";}
};

class ToolSelectAssociatedVertices : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::SelectAssociatedVertices(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Associated Vertices";}
		const char* get_tooltip()	{return "Selects vertices that belong to selected edges, faces and volumes.";}
		const char* get_group()		{return "Selection | Vertices";}
};

class ToolSelectAssociatedEdges : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::SelectAssociatedEdges(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Associated Edges";}
		const char* get_tooltip()	{return "Selects edges that belong to selected faces and volumes.";}
		const char* get_group()		{return "Selection | Edges";}
};

class ToolSelectAssociatedFaces : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::SelectAssociatedFaces(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Associated Faces";}
		const char* get_tooltip()	{return "Selects faces that belong to selected volumes.";}
		const char* get_group()		{return "Selection | Faces";}
};

class ToolSelectAll : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::SelectAll(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Select All";}
		const char* get_tooltip()	{return "Selects all vertices, edges ,faces and volumes of the current grid";}
		const char* get_group()		{return "Selection";}
};

class ToolSelectAllVertices : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::SelectAllVertices(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "All Vertices";}
		const char* get_tooltip()	{return "Selects all vertices of the current grid";}
		const char* get_group()		{return "Selection | Vertices";}
};

class ToolDeselectAllVertices : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::DeselectAllVertices(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Deselect All";}
		const char* get_tooltip()	{return "Deselects all vertices of the current grid";}
		const char* get_group()		{return "Selection | Vertices";}
};

class ToolSelectAllEdges : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::SelectAllEdges(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "All Edges";}
		const char* get_tooltip()	{return "Selects all edges of the current grid";}
		const char* get_group()		{return "Selection | Edges";}
};

class ToolDeselectAllEdges : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::DeselectAllEdges(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Deselect All";}
		const char* get_tooltip()	{return "Deselects all edges of the current grid";}
		const char* get_group()		{return "Selection | Edges";}
};

class ToolSelectAllFaces : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::SelectAllFaces(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "All Faces";}
		const char* get_tooltip()	{return "Selects all faces of the current grid";}
		const char* get_group()		{return "Selection | Faces";}
};

class ToolDeselectAllFaces : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::DeselectAllFaces(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Deselect All";}
		const char* get_tooltip()	{return "Deselects all faces of the current grid";}
		const char* get_group()		{return "Selection | Faces";}
};

class ToolSelectAllVolumes : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::SelectAllVolumes(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "All Volumes";}
		const char* get_tooltip()	{return "Selects all volumes of the current grid";}
		const char* get_group()		{return "Selection | Volumes";}
};

class ToolDeselectAllVolumes : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::DeselectAllVolumes(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Deselect All";}
		const char* get_tooltip()	{return "Deselects all volumes of the current grid";}
		const char* get_group()		{return "Selection | Volumes";}
};

class ToolSelectMarkedVertices : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::SelectMarkedVertices(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Marked Vertices";}
		const char* get_tooltip()	{return "Selects vertices which are marked.";}
		const char* get_group()		{return "Selection | Vertices";}
};

class ToolSelectMarkedEdges : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			promesh::SelectMarkedEdges(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Marked Edges";}
		const char* get_tooltip()	{return "Selects edges which are marked.";}
		const char* get_group()		{return "Selection | Edges";}
};

class ToolSelectUnorientableVolumes : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
			int numUnorientable = promesh::SelectUnorientableVolumes(obj);
			UG_LOG("Unorientable volumes: " << numUnorientable << "\n");
			obj->selection_changed();
		}

		const char* get_name()		{return "Unorientable Volumes";}
		const char* get_tooltip()	{return "Selects all volumes whose orientation can not be determined";}
		const char* get_group()		{return "Selection | Volumes";}
};

class ToolExtendSelection : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);

			int neighborhoodSize = 1;
			if(dlg){
				neighborhoodSize = dlg->to_int(0);
			}

			promesh::ExtendSelection(obj, neighborhoodSize);

			obj->selection_changed();
		}

		const char* get_name()		{return "Extend Selection";}
		const char* get_tooltip()	{return "Selects neighbors of selected elements.";}
		const char* get_group()		{return "Selection";}

		ToolWidget* get_dialog(QWidget* parent){
			ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
											IDB_APPLY | IDB_OK | IDB_CLOSE);
			dlg->addSpinBox(tr("neighborhood size:"), 0, 1.e9, 1, 1, 0);
			return dlg;
		}
};

class ToolSelectVertexByIndex : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);
			if(dlg){
				int index = dlg->to_int(0);

				promesh::SelectVertexByIndex(obj, index);

				obj->selection_changed();
			}
		}

		const char* get_name()		{return "Vertex By Index";}
		const char* get_tooltip()	{return "Selects a vertex given its index.";}
		const char* get_group()		{return "Selection | Vertices";}

		ToolWidget* get_dialog(QWidget* parent){
			ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
											IDB_APPLY | IDB_OK | IDB_CLOSE);
			dlg->addSpinBox(tr("index:"), 0, 1000000000000., 0, 1, 0);
			return dlg;
		}
};

class ToolSelectEdgeByIndex : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);
			if(dlg){
				int index = dlg->to_int(0);
				promesh::SelectEdgeByIndex(obj, index);
				obj->selection_changed();
			}
		}

		const char* get_name()		{return "Edge By Index";}
		const char* get_tooltip()	{return "Selects a edge given its index.";}
		const char* get_group()		{return "Selection | Edges";}

		ToolWidget* get_dialog(QWidget* parent){
			ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
											IDB_APPLY | IDB_OK | IDB_CLOSE);
			dlg->addSpinBox(tr("index:"), 0, 1000000000000., 0, 1, 0);
			return dlg;
		}
};

class ToolSelectFaceByIndex : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);
			if(dlg){
				int index = dlg->to_int(0);
				promesh::SelectFaceByIndex(obj, index);
				obj->selection_changed();
			}
		}

		const char* get_name()		{return "Face By Index";}
		const char* get_tooltip()	{return "Selects a face given its index.";}
		const char* get_group()		{return "Selection | Faces";}

		ToolWidget* get_dialog(QWidget* parent){
			ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
											IDB_APPLY | IDB_OK | IDB_CLOSE);
			dlg->addSpinBox(tr("index:"), 0, 1000000000000., 0, 1, 0);
			return dlg;
		}
};

class ToolSelectVolumeByIndex : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);
			if(dlg){
				int index = dlg->to_int(0);
				promesh::SelectVolumeByIndex(obj, index);
				obj->selection_changed();
			}
		}

		const char* get_name()		{return "Volume By Index";}
		const char* get_tooltip()	{return "Selects a volume given its index.";}
		const char* get_group()		{return "Selection | Volumes";}

		ToolWidget* get_dialog(QWidget* parent){
			ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
											IDB_APPLY | IDB_OK | IDB_CLOSE);
			dlg->addSpinBox(tr("index:"), 0, 1000000000000., 0, 1, 0);
			return dlg;
		}
};

class ToolSelectVertexByCoordinate : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			using namespace ug;
			CoordinatesWidget* dlg = dynamic_cast<CoordinatesWidget*>(widget);
			if(dlg){
				vector3 coord(dlg->x(), dlg->y(), dlg->z());
				VertexBase* e = promesh::SelectElemByCoordinate<VertexBase>(obj, coord);

				if(e){
				//	This message is important, since the user can see
				//	whether the correct vertex was chosen.
					UG_LOG("Selected vertex at: "
							<< CalculateCenter(e, obj->position_accessor()) << "\n");
					obj->selection_changed();
				}
				else{
					UG_LOG("No matching vertex found\n");
				}
			}
		}

		const char* get_name()		{return "Vertex By Coordinate";}
		const char* get_tooltip()	{return "Selects a vertex given a coordinate.";}
		const char* get_group()		{return "Selection | Vertices";}

		QWidget* get_dialog(QWidget* parent){
			return new CoordinatesWidget(get_name(), parent, this, false);
		}
};

class ToolSelectEdgeByCoordinate : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			using namespace ug;
			CoordinatesWidget* dlg = dynamic_cast<CoordinatesWidget*>(widget);
			if(dlg){
				vector3 coord(dlg->x(), dlg->y(), dlg->z());
				EdgeBase* e = promesh::SelectElemByCoordinate<EdgeBase>(obj, coord);

				if(e){
				//	This message is important, since the user can see
				//	whether the correct edge was chosen.
					UG_LOG("Selected edge at: "
							<< CalculateCenter(e, obj->position_accessor()) << "\n");
					obj->selection_changed();
				}
				else{
					UG_LOG("No matching edge found\n");
				}
			}
		}

		const char* get_name()		{return "Edge By Coordinate";}
		const char* get_tooltip()	{return "Selects the edge whose center is closest to the specified coordinate.";}
		const char* get_group()		{return "Selection | Edges";}

		QWidget* get_dialog(QWidget* parent){
			return new CoordinatesWidget(get_name(), parent, this, false);
		}
};

class ToolSelectFaceByCoordinate : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			using namespace ug;
			CoordinatesWidget* dlg = dynamic_cast<CoordinatesWidget*>(widget);
			if(dlg){
				vector3 coord(dlg->x(), dlg->y(), dlg->z());
				Face* e = promesh::SelectElemByCoordinate<Face>(obj, coord);

				if(e){
				//	This message is important, since the user can see
				//	whether the correct face was chosen.
					UG_LOG("Selected face at: "
							<< CalculateCenter(e, obj->position_accessor()) << "\n");
					obj->selection_changed();
				}
				else{
					UG_LOG("No matching face found\n");
				}
			}
		}

		const char* get_name()		{return "Face By Coordinate";}
		const char* get_tooltip()	{return "Selects the face whose center is closest to the specified coordinate.";}
		const char* get_group()		{return "Selection | Faces";}

		QWidget* get_dialog(QWidget* parent){
			return new CoordinatesWidget(get_name(), parent, this, false);
		}
};

class ToolSelectVolumeByCoordinate : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			using namespace ug;
			CoordinatesWidget* dlg = dynamic_cast<CoordinatesWidget*>(widget);
			if(dlg){
				vector3 coord(dlg->x(), dlg->y(), dlg->z());
				Volume* e = promesh::SelectElemByCoordinate<Volume>(obj, coord);

				if(e){
				//	This message is important, since the user can see
				//	whether the correct volume was chosen.
					UG_LOG("Selected volume at: "
							<< CalculateCenter(e, obj->position_accessor()) << "\n");
					obj->selection_changed();
				}
				else{
					UG_LOG("No matching volume found\n");
				}
			}
		}

		const char* get_name()		{return "Volume By Coordinate";}
		const char* get_tooltip()	{return "Selects the volume whose center is closest to the specified coordinate.";}
		const char* get_group()		{return "Selection | Volumes";}

		QWidget* get_dialog(QWidget* parent){
			return new CoordinatesWidget(get_name(), parent, this, false);
		}
};

class ToolSelectUnconnectedVertices : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			using namespace ug;
			ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);
			if(dlg){
				int choice = dlg->to_int(0);
				bool edgeCons = (choice == 0);
				bool faceCons = (choice == 1);
				bool volCons = (choice == 2);

				size_t numUnconnected = promesh::SelectUnconnectedVertices(obj, edgeCons,
																		   faceCons, volCons);

				UG_LOG(numUnconnected << " unconnected vertices found.\n");
				if(numUnconnected > 0)
					obj->selection_changed();
			}
		}

		const char* get_name()		{return "Unconnected Vertices";}
		const char* get_tooltip()	{return "Selects vertices which are not connected to the given element type.";}
		const char* get_group()		{return "Selection | Vertices";}

		ToolWidget* get_dialog(QWidget* parent){
			ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
											IDB_APPLY | IDB_OK | IDB_CLOSE);
			QStringList entries;
			entries.push_back("edges");
			entries.push_back("faces");
			entries.push_back("volumes");

			dlg->addComboBox("not connected to:", entries, 0);
			return dlg;
		}
};

class ToolSelectSubset : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);
			if(dlg){
				bool selVrts = true;
				bool selEdges = true;
				bool selFaces = true;
				bool selVols = true;
				int si = 0;

				if(dlg){
					selVrts = dlg->to_bool(0);
					selEdges = dlg->to_bool(1);
					selFaces = dlg->to_bool(2);
					selVols = dlg->to_bool(3);
					si = dlg->to_int(4);
				}

				promesh::SelectSubset(obj, si, selVrts, selEdges, selFaces, selVols);
				obj->selection_changed();
			}
		}

		const char* get_name()		{return "Subset";}
		const char* get_tooltip()	{return "Selects all elements of a subset.";}
		const char* get_group()		{return "Selection";}

		ToolWidget* get_dialog(QWidget* parent){
			ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
											IDB_APPLY | IDB_OK | IDB_CLOSE);
			ToolDialog_AddElementCheckBoxes(dlg, TDUE_ALL);
			dlg->addSpinBox(tr("subset index:"), -1, 1e9, 0, 1, 0);
			return dlg;
		}
};

class ToolSelectSubsetBoundary : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			using namespace ug;
			ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);
			if(dlg){
				bool edgeBnds = true;
				bool faceBnds = true;
				bool volBnds = true;
				int si = 0;

				if(dlg){
					edgeBnds = dlg->to_bool(0);
					faceBnds = dlg->to_bool(1);
					volBnds = dlg->to_bool(2);
					si = dlg->to_int(3);
				}

				promesh::SelectSubsetBoundary(obj, si, edgeBnds, faceBnds, volBnds);

				obj->selection_changed();
			}
		}

		const char* get_name()		{return "Subset Boundary";}
		const char* get_tooltip()	{return "Selects the boundary of a subset.";}
		const char* get_group()		{return "Selection";}

		ToolWidget* get_dialog(QWidget* parent){
			ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
											IDB_APPLY | IDB_OK | IDB_CLOSE);
			dlg->addCheckBox("bounds of edges", true);
			dlg->addCheckBox("bounds of faces", true);
			dlg->addCheckBox("bounds of volumes", true);
			dlg->addSpinBox(tr("subset index:"), -1, 1e9, 0, 1, 0);
			return dlg;
		}
};


class ToolSelectUnassignedElements : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);
			if(dlg){
				bool selVrts = true;
				bool selEdges = true;
				bool selFaces = true;
				bool selVols = true;

				if(dlg){
					selVrts = dlg->to_bool(0);
					selEdges = dlg->to_bool(1);
					selFaces = dlg->to_bool(2);
					selVols = dlg->to_bool(3);
				}

				promesh::SelectUnassignedElements(obj, selVrts, selEdges, selFaces, selVols);

				obj->selection_changed();
			}
		}

		const char* get_name()		{return "Unassigned Elements";}
		const char* get_tooltip()	{return "Selects all elements not assigned to any subset.";}
		const char* get_group()		{return "Selection";}

		ToolWidget* get_dialog(QWidget* parent){
			ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
											IDB_APPLY | IDB_OK | IDB_CLOSE);
			ToolDialog_AddElementCheckBoxes(dlg, TDUE_ALL);
			return dlg;
		}
};

class ToolInvertSelection : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			using namespace ug;
			ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);
			if(dlg){
				bool invVrts = true;
				bool invEdges = true;
				bool invFaces = true;
				bool invVols = true;

				if(dlg){
					invVrts = dlg->to_bool(0);
					invEdges = dlg->to_bool(1);
					invFaces = dlg->to_bool(2);
					invVols = dlg->to_bool(3);
				}

				promesh::InvertSelection(obj, invVrts, invEdges, invFaces, invVols);

				obj->selection_changed();
			}
		}

		const char* get_name()		{return "Invert Selection";}
		const char* get_tooltip()	{return "Inverts current selection.";}
		const char* get_group()		{return "Selection";}

		ToolWidget* get_dialog(QWidget* parent){
			ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
											IDB_APPLY | IDB_OK | IDB_CLOSE);
			ToolDialog_AddElementCheckBoxes(dlg, TDUE_ALL);
			return dlg;
		}
};

class ToolEdgeSelectionFill : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*)
		{
			promesh::EdgeSelectionFill(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Edge Selection Fill";}
		const char* get_tooltip()	{return "Selects neighbours of selected edges over non-selected vertices.";}
		const char* get_group()		{return "Selection | Edges";}
};

class ToolFaceSelectionFill : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*)
		{
			promesh::FaceSelectionFill(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Face Selection Fill";}
		const char* get_tooltip()	{return "Selects neighbours of selected faces over non-selected edges.";}
		const char* get_group()		{return "Selection | Faces";}
};

class ToolVolumeSelectionFill : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*)
		{
			promesh::VolumeSelectionFill(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Volume Selection Fill";}
		const char* get_tooltip()	{return "Selects neighbours of selected volumes over non-selected faces.";}
		const char* get_group()		{return "Selection | Volumes";}
};

class ToolSelectSelectionBoundary : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*)
		{
			promesh::SelectSelectionBoundary(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Selection Boundary";}
		const char* get_tooltip()	{return "Selects the boundary of the current selection.";}
		const char* get_group()		{return "Selection";}
};


class ToolSelectBentQuadrilaterals: public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget)
		{
			ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);

			number dotThreshold = 0.95;
			if(dlg){
				dotThreshold = dlg->to_double(0);
			}

			size_t selCount = promesh::SelectBentQuadrilaterals(obj, dotThreshold);

			UG_LOG("Found " << selCount << " bent quadrilaterals\n");
			obj->selection_changed();
		}

		const char* get_name()		{return "Bent Quadrilaterals";}
		const char* get_tooltip()	{return "Selects quadrilaterals which do not lie in a plane.";}
		const char* get_group()		{return "Selection | Faces";}

		ToolWidget* get_dialog(QWidget* parent){
			ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
											IDB_APPLY | IDB_OK | IDB_CLOSE);
			dlg->addSpinBox(tr("dot-threshold"), -1, 1, 0.95, 0.01, 9);
			return dlg;
		}
};

class ToolCloseSelection: public ITool
{
	public:
		void execute(LGObject* obj, QWidget*)
		{
			promesh::CloseSelection(obj);
			obj->selection_changed();
		}

		const char* get_name()		{return "Close Selection";}
		const char* get_tooltip()	{return "Selects all associated elements of lower dimensions.";}
		const char* get_group()		{return "Selection";}
};


void RegisterSelectionTools(ToolManager* toolMgr)
{
	toolMgr->set_group_icon("Selection", ":images/tool_selection.png");

//	GENERAL TOOLS
	toolMgr->register_tool(new ToolClearSelection);
	toolMgr->register_tool(new ToolSelectAll);
	toolMgr->register_tool(new ToolCloseSelection, Qt::Key_C);
	toolMgr->register_tool(new ToolSelectSelectionBoundary);

//	VERTICES
	toolMgr->register_tool(new ToolSelectAllVertices);
	toolMgr->register_tool(new ToolDeselectAllVertices);
	toolMgr->register_tool(new ToolSelectAssociatedVertices);
	toolMgr->register_tool(new ToolSelectMarkedVertices);
	toolMgr->register_tool(new ToolSelectInnerVertices);
	toolMgr->register_tool(new ToolSelectBoundaryVertices);

	toolMgr->register_tool(new ToolSelectVertexByIndex);
	toolMgr->register_tool(new ToolSelectVertexByCoordinate);
	toolMgr->register_tool(new ToolSelectUnconnectedVertices);

//	EDGES
	toolMgr->register_tool(new ToolSelectAllEdges);
	toolMgr->register_tool(new ToolDeselectAllEdges);
	toolMgr->register_tool(new ToolSelectAssociatedEdges);
	toolMgr->register_tool(new ToolSelectMarkedEdges);
	toolMgr->register_tool(new ToolEdgeSelectionFill);
	toolMgr->register_tool(new ToolSelectInnerEdges);
	toolMgr->register_tool(new ToolSelectBoundaryEdges);
	toolMgr->register_tool(new ToolSelectNonManifoldEdges);

	toolMgr->register_tool(new ToolSelectLinkedBoundaryEdges);
	toolMgr->register_tool(new ToolSelectSmoothEdgePath);
	toolMgr->register_tool(new ToolSelectShortEdges);
	toolMgr->register_tool(new ToolSelectLongEdges);
	toolMgr->register_tool(new ToolSelectEdgeByIndex);
	toolMgr->register_tool(new ToolSelectEdgeByCoordinate);

//	FACES
	toolMgr->register_tool(new ToolSelectAllFaces);
	toolMgr->register_tool(new ToolDeselectAllFaces);
	toolMgr->register_tool(new ToolSelectAssociatedFaces);
	toolMgr->register_tool(new ToolFaceSelectionFill);
	toolMgr->register_tool(new ToolSelectIntersectingTriangles);
	toolMgr->register_tool(new ToolSelectInnerFaces);
	toolMgr->register_tool(new ToolSelectBoundaryFaces);
	toolMgr->register_tool(new ToolSelectLinkedManifoldFaces);

	toolMgr->register_tool(new ToolSelectLinkedBoundaryFaces);
	toolMgr->register_tool(new ToolSelectLinkedFlatFaces);
	toolMgr->register_tool(new ToolSelectDegenerateFaces);
	toolMgr->register_tool(new ToolSelectFaceByIndex);
	toolMgr->register_tool(new ToolSelectFaceByCoordinate);
	toolMgr->register_tool(new ToolSelectBentQuadrilaterals);

//	VOLUMES
	toolMgr->register_tool(new ToolSelectAllVolumes);
	toolMgr->register_tool(new ToolDeselectAllVolumes);
	toolMgr->register_tool(new ToolVolumeSelectionFill);
	toolMgr->register_tool(new ToolSelectUnorientableVolumes);

	toolMgr->register_tool(new ToolSelectVolumeByIndex);
	toolMgr->register_tool(new ToolSelectVolumeByCoordinate);

//	MORE GENERAL TOOLS
	toolMgr->register_tool(new ToolInvertSelection);
	toolMgr->register_tool(new ToolExtendSelection);
	toolMgr->register_tool(new ToolSelectSubset);
	toolMgr->register_tool(new ToolSelectSubsetBoundary);
	toolMgr->register_tool(new ToolSelectUnassignedElements);
}
