add_qtc_plugin(Todo
  DEPENDS QmlJS
  PLUGIN_DEPENDS Core CppEditor ProjectExplorer
  SOURCES
    constants.h
    cpptodoitemsscanner.cpp cpptodoitemsscanner.h
    keyword.cpp keyword.h
    keyworddialog.cpp keyworddialog.h
    lineparser.cpp lineparser.h
    optionsdialog.cpp optionsdialog.h
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
    todoplugin.cpp todoplugin.h
    todoplugin.qrc
    todoprojectsettingswidget.cpp todoprojectsettingswidget.h
    todotr.h
)
