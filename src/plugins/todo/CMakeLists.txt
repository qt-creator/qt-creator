add_qtc_plugin(Todo
  DEPENDS QmlJS
  PLUGIN_DEPENDS Core CppEditor ProjectExplorer
  SOURCES
    constants.h
    cpptodoitemsscanner.cpp cpptodoitemsscanner.h
    keyword.cpp keyword.h
    keyworddialog.cpp keyworddialog.h
    lineparser.cpp lineparser.h
    projectfiletodoitemsscanner.cpp projectfiletodoitemsscanner.h
    qmljstodoitemsscanner.cpp qmljstodoitemsscanner.h
    settings.cpp settings.h
    todoicons.cpp todoicons.h
    todoitem.h
    todoitemsmodel.cpp todoitemsmodel.h
    todoitemsprovider.cpp todoitemsprovider.h
    todoitemsscanner.cpp todoitemsscanner.h
    todooutputpane.cpp todooutputpane.h
    todooutputtreeview.cpp todooutputtreeview.h
    todooutputtreeviewdelegate.cpp todooutputtreeviewdelegate.h
    todoplugin.cpp
    todoprojectpanel.cpp todoprojectpanel.h
    todotr.h
)

qtc_add_resources(Todo "images"
  PREFIX "/todoplugin"
  BASE "."
  FILES
    images/settingscategory_todo.png
    images/settingscategory_todo@2x.png
    images/tasklist@2x.png
    images/tasklist.png
    images/bug@2x.png
    images/bug.png
    images/bugfill.png
    images/bugfill@2x.png
)
