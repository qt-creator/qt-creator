add_qtc_plugin(Help
  CONDITION TARGET Qt5::Help
  DEPENDS shared_help
  PLUGIN_DEPENDS Core ProjectExplorer
  PLUGIN_RECOMMENDS TextEditor
  SOURCES
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
    openpagesswitcher.cpp openpagesswitcher.h
    openpageswidget.cpp openpageswidget.h
    searchtaskhandler.cpp searchtaskhandler.h
    searchwidget.cpp searchwidget.h
    textbrowserhelpviewer.cpp textbrowserhelpviewer.h
    xbelsupport.cpp xbelsupport.h
)

extend_qtc_plugin(Help
  CONDITION Qt5_VERSION VERSION_GREATER_EQUAL 5.15.0
  DEFINES HELP_NEW_FILTER_ENGINE
)

extend_qtc_plugin(Help
  CONDITION FWWebKit AND FWAppKit
  FEATURE_INFO "Native WebKit help viewer"
  DEPENDS ${FWWebKit} ${FWAppKit}
  DEFINES QTC_MAC_NATIVE_HELPVIEWER
  SOURCES
    macwebkithelpviewer.h
    macwebkithelpviewer.mm
)

find_package(Qt5WebEngineWidgets QUIET)
extend_qtc_plugin(Help
  CONDITION TARGET Qt5::WebEngineWidgets
  FEATURE_INFO "QtWebEngine help viewer"
  DEPENDS Qt5::WebEngineWidgets
  DEFINES QTC_WEBENGINE_HELPVIEWER
  SOURCES
    webenginehelpviewer.cpp
    webenginehelpviewer.h
)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/qlitehtml/litehtml/CMakeLists.txt)
  add_subdirectory(qlitehtml)
else()
  find_package(litehtml QUIET)
  if (TARGET litehtml)
    add_subdirectory(qlitehtml)
  endif()
endif()

extend_qtc_plugin(Help
  CONDITION TARGET litehtml AND TARGET qlitehtml
  FEATURE_INFO "litehtml help viewer"
  DEPENDS qlitehtml
  DEFINES QTC_LITEHTML_HELPVIEWER
  SOURCES
    litehtmlhelpviewer.cpp
    litehtmlhelpviewer.h
)
