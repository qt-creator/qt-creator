add_qtc_library(Modeling
  DEFINES MODELING_LIBRARY
  DEPENDS Qt5::Widgets Utils
  PUBLIC_DEPENDS OptionalSvg
  INCLUDES qtserialization/inc
  PUBLIC_INCLUDES
    "${CMAKE_CURRENT_LIST_DIR}"
  SOURCES
    qmt/config/configcontroller.cpp qmt/config/configcontroller.h
    qmt/config/sourcepos.cpp qmt/config/sourcepos.h
    qmt/config/stereotypedefinitionparser.cpp qmt/config/stereotypedefinitionparser.h
    qmt/config/stringtextsource.cpp qmt/config/stringtextsource.h
    qmt/config/textscanner.cpp qmt/config/textscanner.h
    qmt/config/textsource.h
    qmt/config/token.cpp qmt/config/token.h
    qmt/controller/container.h
    qmt/controller/namecontroller.cpp qmt/controller/namecontroller.h
    qmt/controller/references.h
    qmt/controller/selection.cpp qmt/controller/selection.h
    qmt/controller/undocommand.cpp qmt/controller/undocommand.h
    qmt/controller/undocontroller.cpp qmt/controller/undocontroller.h
    qmt/diagram_controller/dclonevisitor.cpp qmt/diagram_controller/dclonevisitor.h
    qmt/diagram_controller/dcontainer.h
    qmt/diagram_controller/dfactory.cpp qmt/diagram_controller/dfactory.h
    qmt/diagram_controller/dflatassignmentvisitor.cpp qmt/diagram_controller/dflatassignmentvisitor.h
    qmt/diagram_controller/diagramcontroller.cpp qmt/diagram_controller/diagramcontroller.h
    qmt/diagram_controller/dreferences.h
    qmt/diagram_controller/dselection.h
    qmt/diagram_controller/dupdatevisitor.cpp qmt/diagram_controller/dupdatevisitor.h
    qmt/diagram_controller/dvoidvisitor.cpp qmt/diagram_controller/dvoidvisitor.h
    qmt/diagram/dannotation.cpp qmt/diagram/dannotation.h
    qmt/diagram/dassociation.cpp qmt/diagram/dassociation.h
    qmt/diagram/dboundary.cpp qmt/diagram/dboundary.h
    qmt/diagram/dclass.cpp qmt/diagram/dclass.h
    qmt/diagram/dcomponent.cpp qmt/diagram/dcomponent.h
    qmt/diagram/dconnection.cpp qmt/diagram/dconnection.h
    qmt/diagram/dconstvisitor.h
    qmt/diagram/ddependency.cpp qmt/diagram/ddependency.h
    qmt/diagram/ddiagram.cpp qmt/diagram/ddiagram.h
    qmt/diagram/delement.cpp qmt/diagram/delement.h
    qmt/diagram/dinheritance.cpp qmt/diagram/dinheritance.h
    qmt/diagram/ditem.cpp qmt/diagram/ditem.h
    qmt/diagram/dobject.cpp qmt/diagram/dobject.h
    qmt/diagram/dpackage.cpp qmt/diagram/dpackage.h
    qmt/diagram/drelation.cpp qmt/diagram/drelation.h
    qmt/diagram/dswimlane.cpp qmt/diagram/dswimlane.h
    qmt/diagram/dvisitor.h
    qmt/diagram_scene/capabilities/alignable.h
    qmt/diagram_scene/capabilities/editable.h
    qmt/diagram_scene/capabilities/intersectionable.h
    qmt/diagram_scene/capabilities/latchable.h
    qmt/diagram_scene/capabilities/moveable.h
    qmt/diagram_scene/capabilities/relationable.h
    qmt/diagram_scene/capabilities/resizable.h
    qmt/diagram_scene/capabilities/selectable.h
    qmt/diagram_scene/capabilities/windable.h
    qmt/diagram_scene/diagramgraphicsscene.cpp qmt/diagram_scene/diagramgraphicsscene.h
    qmt/diagram_scene/diagramsceneconstants.h
    qmt/diagram_scene/diagramscenemodel.cpp qmt/diagram_scene/diagramscenemodel.h
    qmt/diagram_scene/diagramscenemodelitemvisitors.cpp qmt/diagram_scene/diagramscenemodelitemvisitors.h
    qmt/diagram_scene/items/annotationitem.cpp qmt/diagram_scene/items/annotationitem.h
    qmt/diagram_scene/items/associationitem.cpp qmt/diagram_scene/items/associationitem.h
    qmt/diagram_scene/items/boundaryitem.cpp qmt/diagram_scene/items/boundaryitem.h
    qmt/diagram_scene/items/classitem.cpp qmt/diagram_scene/items/classitem.h
    qmt/diagram_scene/items/componentitem.cpp qmt/diagram_scene/items/componentitem.h
    qmt/diagram_scene/items/connectionitem.cpp qmt/diagram_scene/items/connectionitem.h
    qmt/diagram_scene/items/diagramitem.cpp qmt/diagram_scene/items/diagramitem.h
    qmt/diagram_scene/items/itemitem.cpp qmt/diagram_scene/items/itemitem.h
    qmt/diagram_scene/items/objectitem.cpp qmt/diagram_scene/items/objectitem.h
    qmt/diagram_scene/items/packageitem.cpp qmt/diagram_scene/items/packageitem.h
    qmt/diagram_scene/items/relationitem.cpp qmt/diagram_scene/items/relationitem.h
    qmt/diagram_scene/items/stereotypedisplayvisitor.cpp qmt/diagram_scene/items/stereotypedisplayvisitor.h
    qmt/diagram_scene/items/swimlaneitem.cpp qmt/diagram_scene/items/swimlaneitem.h
    qmt/diagram_scene/latchcontroller.cpp qmt/diagram_scene/latchcontroller.h
    qmt/diagram_scene/parts/alignbuttonsitem.cpp qmt/diagram_scene/parts/alignbuttonsitem.h
    qmt/diagram_scene/parts/alignlineitem.cpp qmt/diagram_scene/parts/alignlineitem.h
    qmt/diagram_scene/parts/arrowitem.cpp qmt/diagram_scene/parts/arrowitem.h
    qmt/diagram_scene/parts/contextlabelitem.cpp qmt/diagram_scene/parts/contextlabelitem.h
    qmt/diagram_scene/parts/customiconitem.cpp qmt/diagram_scene/parts/customiconitem.h
    qmt/diagram_scene/parts/editabletextitem.cpp qmt/diagram_scene/parts/editabletextitem.h
    qmt/diagram_scene/parts/pathselectionitem.cpp qmt/diagram_scene/parts/pathselectionitem.h
    qmt/diagram_scene/parts/rectangularselectionitem.cpp qmt/diagram_scene/parts/rectangularselectionitem.h
    qmt/diagram_scene/parts/relationstarter.cpp qmt/diagram_scene/parts/relationstarter.h
    qmt/diagram_scene/parts/stereotypesitem.cpp qmt/diagram_scene/parts/stereotypesitem.h
    qmt/diagram_scene/parts/templateparameterbox.cpp qmt/diagram_scene/parts/templateparameterbox.h
    qmt/diagram_ui/diagram_mime_types.h
    qmt/diagram_ui/diagramsmanager.cpp qmt/diagram_ui/diagramsmanager.h
    qmt/diagram_ui/diagramsviewinterface.h
    qmt/diagram_ui/sceneinspector.cpp qmt/diagram_ui/sceneinspector.h
    qmt/diagram_widgets_ui/diagramsview.cpp qmt/diagram_widgets_ui/diagramsview.h
    qmt/diagram_widgets_ui/diagramview.cpp qmt/diagram_widgets_ui/diagramview.h
    qmt/diagram_widgets_ui/stackeddiagramsview.cpp qmt/diagram_widgets_ui/stackeddiagramsview.h
    qmt/document_controller/documentcontroller.cpp qmt/document_controller/documentcontroller.h
    qmt/infrastructure/contextmenuaction.cpp qmt/infrastructure/contextmenuaction.h
    qmt/infrastructure/exceptions.cpp qmt/infrastructure/exceptions.h
    qmt/infrastructure/geometryutilities.cpp qmt/infrastructure/geometryutilities.h
    qmt/infrastructure/handle.h
    qmt/infrastructure/handles.h
    qmt/infrastructure/ioexceptions.cpp qmt/infrastructure/ioexceptions.h
    qmt/infrastructure/qcompressedfile.cpp qmt/infrastructure/qcompressedfile.h
    qmt/infrastructure/qmtassert.h
    qmt/infrastructure/qmt_global.h
    qmt/infrastructure/uid.h
    qmt/model_controller/mchildrenvisitor.cpp qmt/model_controller/mchildrenvisitor.h
    qmt/model_controller/mclonevisitor.cpp qmt/model_controller/mclonevisitor.h
    qmt/model_controller/mcontainer.h
    qmt/model_controller/mflatassignmentvisitor.cpp qmt/model_controller/mflatassignmentvisitor.h
    qmt/model_controller/modelcontroller.cpp qmt/model_controller/modelcontroller.h
    qmt/model_controller/mreferences.h
    qmt/model_controller/mselection.h
    qmt/model_controller/mvoidvisitor.cpp qmt/model_controller/mvoidvisitor.h
    qmt/model/massociation.cpp qmt/model/massociation.h
    qmt/model/mcanvasdiagram.cpp qmt/model/mcanvasdiagram.h
    qmt/model/mclass.cpp qmt/model/mclass.h
    qmt/model/mclassmember.cpp qmt/model/mclassmember.h
    qmt/model/mcomponent.cpp qmt/model/mcomponent.h
    qmt/model/mconnection.cpp qmt/model/mconnection.h
    qmt/model/mconstvisitor.h
    qmt/model/mdependency.cpp qmt/model/mdependency.h
    qmt/model/mdiagram.cpp qmt/model/mdiagram.h
    qmt/model/melement.cpp qmt/model/melement.h
    qmt/model/minheritance.cpp qmt/model/minheritance.h
    qmt/model/mitem.cpp qmt/model/mitem.h
    qmt/model/mobject.cpp qmt/model/mobject.h
    qmt/model/mpackage.cpp qmt/model/mpackage.h
    qmt/model/mrelation.cpp qmt/model/mrelation.h
    qmt/model/msourceexpansion.cpp qmt/model/msourceexpansion.h
    qmt/model/mvisitor.h
    qmt/model_ui/modeltreeviewinterface.h
    qmt/model_ui/sortedtreemodel.cpp qmt/model_ui/sortedtreemodel.h
    qmt/model_ui/stereotypescontroller.cpp qmt/model_ui/stereotypescontroller.h
    qmt/model_ui/treemodel.cpp qmt/model_ui/treemodel.h
    qmt/model_ui/treemodelmanager.cpp qmt/model_ui/treemodelmanager.h
    qmt/model_widgets_ui/classmembersedit.cpp qmt/model_widgets_ui/classmembersedit.h
    qmt/model_widgets_ui/modeltreeview.cpp qmt/model_widgets_ui/modeltreeview.h
    qmt/model_widgets_ui/palettebox.cpp qmt/model_widgets_ui/palettebox.h
    qmt/model_widgets_ui/propertiesview.cpp qmt/model_widgets_ui/propertiesview.h
    qmt/model_widgets_ui/propertiesviewmview.cpp qmt/model_widgets_ui/propertiesviewmview.h
    qmt/project_controller/projectcontroller.cpp qmt/project_controller/projectcontroller.h
    qmt/project/project.cpp qmt/project/project.h
    qmt/resources/resources.qrc
    qmt/serializer/diagramserializer.cpp qmt/serializer/diagramserializer.h
    qmt/serializer/infrastructureserializer.cpp qmt/serializer/infrastructureserializer.h
    qmt/serializer/modelserializer.cpp qmt/serializer/modelserializer.h
    qmt/serializer/projectserializer.cpp qmt/serializer/projectserializer.h
    qmt/stereotype/customrelation.cpp qmt/stereotype/customrelation.h
    qmt/stereotype/iconshape.cpp qmt/stereotype/iconshape.h
    qmt/stereotype/shape.h
    qmt/stereotype/shapepaintvisitor.cpp qmt/stereotype/shapepaintvisitor.h
    qmt/stereotype/shapes.cpp qmt/stereotype/shapes.h
    qmt/stereotype/shapevalue.cpp qmt/stereotype/shapevalue.h
    qmt/stereotype/shapevisitor.h
    qmt/stereotype/stereotypecontroller.cpp qmt/stereotype/stereotypecontroller.h
    qmt/stereotype/stereotypeicon.cpp qmt/stereotype/stereotypeicon.h
    qmt/stereotype/toolbar.cpp qmt/stereotype/toolbar.h
    qmt/style/defaultstyle.cpp
    qmt/style/defaultstyleengine.cpp qmt/style/defaultstyleengine.h
    qmt/style/defaultstyle.h
    qmt/style/objectvisuals.cpp qmt/style/objectvisuals.h
    qmt/style/relationstarterstyle.cpp qmt/style/relationstarterstyle.h
    qmt/style/stylecontroller.cpp qmt/style/stylecontroller.h
    qmt/style/style.cpp
    qmt/style/styledobject.cpp qmt/style/styledobject.h
    qmt/style/styledrelation.cpp qmt/style/styledrelation.h
    qmt/style/styleengine.h
    qmt/style/style.h
    qmt/tasks/alignonrastervisitor.cpp qmt/tasks/alignonrastervisitor.h
    qmt/tasks/diagramscenecontroller.cpp qmt/tasks/diagramscenecontroller.h
    qmt/tasks/finddiagramvisitor.cpp qmt/tasks/finddiagramvisitor.h
    qmt/tasks/findrootdiagramvisitor.cpp qmt/tasks/findrootdiagramvisitor.h
    qmt/tasks/ielementtasks.h
    qmt/tasks/isceneinspector.h
    qmt/tasks/voidelementtasks.cpp qmt/tasks/voidelementtasks.h
    qstringparser/qstringparser.cpp qstringparser/qstringparser.h
    qtserialization/inc/qark/access.h
    qtserialization/inc/qark/archivebasics.h
    qtserialization/inc/qark/attribute.h
    qtserialization/inc/qark/baseclass.h
    qtserialization/inc/qark/flag.h
    qtserialization/inc/qark/friend_access.h
    qtserialization/inc/qark/impl/loadingrefmap.h
    qtserialization/inc/qark/impl/objectid.h
    qtserialization/inc/qark/impl/savingrefmap.h
    qtserialization/inc/qark/parameters.h
    qtserialization/inc/qark/qxmlinarchive.h
    qtserialization/inc/qark/qxmloutarchive.h
    qtserialization/inc/qark/reference.h
    qtserialization/inc/qark/serialize_basic.h
    qtserialization/inc/qark/serialize_container.h
    qtserialization/inc/qark/serialize_enum.h
    qtserialization/inc/qark/serialize.h
    qtserialization/inc/qark/serialize_pointer.h
    qtserialization/inc/qark/tag.h
    qtserialization/inc/qark/typeregistry.h
    qtserialization/src/flag.cpp
    qtserialization/src/savingrefmap.cpp
)
