add_qtc_plugin(Help
  CONDITION TARGET Qt5::Help
  DEPENDS shared_help
  PLUGIN_DEPENDS Core ProjectExplorer
  PLUGIN_RECOMMENDS TextEditor
  SOURCES
    centralwidget.cpp centralwidget.h
    docsettingspage.cpp docsettingspage.h docsettingspage.ui
    filtersettingspage.cpp filtersettingspage.h filtersettingspage.ui
    generalsettingspage.cpp generalsettingspage.h generalsettingspage.ui
    help.qrc
    helpconstants.h
    helpfindsupport.cpp helpfindsupport.h
    helpindexfilter.cpp helpindexfilter.h
    helpmanager.cpp helpmanager.h
    helpmode.cpp helpmode.h
    helpplugin.cpp helpplugin.h
    helpviewer.cpp helpviewer.h
    helpwidget.cpp helpwidget.h
    localhelpmanager.cpp localhelpmanager.h
    openpagesmanager.cpp openpagesmanager.h
    openpagesmodel.cpp openpagesmodel.h
    openpagesswitcher.cpp openpagesswitcher.h
    openpageswidget.cpp openpageswidget.h
    remotehelpfilter.cpp remotehelpfilter.h remotehelpfilter.ui
    searchtaskhandler.cpp searchtaskhandler.h
    searchwidget.cpp searchwidget.h
    textbrowserhelpviewer.cpp textbrowserhelpviewer.h
    xbelsupport.cpp xbelsupport.h
)

if (APPLE)
  find_library(FWWebKit WebKit)
  find_library(FWAppKit AppKit)
  target_link_libraries(Help PRIVATE ${FWWebKit} ${FWAppKit})
  target_compile_definitions(Help PRIVATE QTC_MAC_NATIVE_HELPVIEWER)
  target_sources(Help PRIVATE macwebkithelpviewer.h macwebkithelpviewer.mm)
endif()

find_package(Qt5WebEngineWidgets QUIET)

if (TARGET Qt5::WebEngineWidgets)
  target_sources(Help PRIVATE webenginehelpviewer.cpp webenginehelpviewer.h)
  target_link_libraries(Help PRIVATE Qt5::WebEngineWidgets)
endif()
