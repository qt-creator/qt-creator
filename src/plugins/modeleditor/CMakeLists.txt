add_qtc_plugin(ModelEditor
  DEPENDS Modeling Qt::Core Qt::Gui Qt::Widgets
  PLUGIN_DEPENDS Core CppEditor ProjectExplorer
  SOURCES
    actionhandler.cpp actionhandler.h
    classviewcontroller.cpp classviewcontroller.h
    componentviewcontroller.cpp componentviewcontroller.h
    diagramsviewmanager.cpp diagramsviewmanager.h
    dragtool.cpp dragtool.h
    editordiagramview.cpp editordiagramview.h
    elementtasks.cpp elementtasks.h
    extdocumentcontroller.cpp extdocumentcontroller.h
    extpropertiesmview.cpp extpropertiesmview.h
    jsextension.cpp jsextension.h
    modeldocument.cpp modeldocument.h
    modeleditor.cpp modeleditor.h
    modeleditor_constants.h
    modeleditor_global.h
    modeleditortr.h
    modeleditor_plugin.cpp modeleditor_plugin.h
    modeleditorfactory.cpp modeleditorfactory.h
    modelindexer.cpp modelindexer.h
    modelsmanager.cpp modelsmanager.h
    modelutilities.cpp modelutilities.h
    openelementvisitor.cpp openelementvisitor.h
    packageviewcontroller.cpp packageviewcontroller.h
    pxnodecontroller.cpp pxnodecontroller.h
    pxnodeutilities.cpp pxnodeutilities.h
    uicontroller.cpp uicontroller.h
  EXPLICIT_MOC
    actionhandler.h
    modeleditor.h
)

qtc_add_resources(ModelEditor "resources"
  PREFIX "/modeleditor"
  BASE "resources"
  FILES
    resources/up.png
    resources/wizards/modeling/model/file.qmodel
    resources/wizards/modeling/model/wizard.json
    resources/wizards/modeling/scratch/file.qmodel
    resources/wizards/modeling/scratch/wizard.json
)
