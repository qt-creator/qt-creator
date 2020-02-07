add_qtc_plugin(Todo
  DEPENDS QmlJS
  PLUGIN_DEPENDS Core CppTools ProjectExplorer
  SOURCES
    constants.h
    cpptodoitemsscanner.cpp cpptodoitemsscanner.h
    keyword.cpp keyword.h
    keyworddialog.cpp keyworddialog.h keyworddialog.ui
    lineparser.cpp lineparser.h
    optionsdialog.cpp optionsdialog.h optionsdialog.ui
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
    todoprojectsettingswidget.cpp todoprojectsettingswidget.h todoprojectsettingswidget.ui
)
