add_qtc_plugin(Help
  CONDITION TARGET Qt::Help
  DEPENDS shared_help
  PLUGIN_DEPENDS Core ProjectExplorer
  PLUGIN_RECOMMENDS TextEditor
  SOURCES
    docsettingspage.cpp docsettingspage.h
    filtersettingspage.cpp filtersettingspage.h
    generalsettingspage.cpp generalsettingspage.h
    help.qrc
    helpconstants.h
    helpfindsupport.cpp helpfindsupport.h
    helpindexfilter.cpp helpindexfilter.h
    helpmanager.cpp helpmanager.h
    helpplugin.cpp helpplugin.h
    helptr.h
    helpviewer.cpp helpviewer.h
    helpwidget.cpp helpwidget.h
    localhelpmanager.cpp localhelpmanager.h
    offline-dark.css
    openpagesmanager.cpp openpagesmanager.h
    openpagesswitcher.cpp openpagesswitcher.h
    openpageswidget.cpp openpageswidget.h
    searchtaskhandler.cpp searchtaskhandler.h
    searchwidget.cpp searchwidget.h
    textbrowserhelpviewer.cpp textbrowserhelpviewer.h
    xbelsupport.cpp xbelsupport.h
)

set(HELPVIEWER_DEFAULT_BACKEND "litehtml" CACHE STRING "Sets default help viewer backend")
set_property(CACHE HELPVIEWER_DEFAULT_BACKEND PROPERTY STRINGS "litehtml;qtwebengine;textbrowser")

extend_qtc_plugin(Help
  CONDITION HELPVIEWER_DEFAULT_BACKEND
  DEFINES QTC_DEFAULT_HELPVIEWER_BACKEND="${HELPVIEWER_DEFAULT_BACKEND}"
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

option(BUILD_HELPVIEWERBACKEND_QTWEBENGINE "Build QtWebEngine based help viewer backend." YES)
find_package(Qt6 COMPONENTS WebEngineWidgets QUIET)
extend_qtc_plugin(Help
  CONDITION BUILD_HELPVIEWERBACKEND_QTWEBENGINE AND TARGET Qt::WebEngineWidgets
  FEATURE_INFO "QtWebEngine help viewer"
  DEPENDS Qt::WebEngineWidgets
  DEFINES QTC_WEBENGINE_HELPVIEWER
  SOURCES
    webenginehelpviewer.cpp
    webenginehelpviewer.h
  EXPLICIT_MOC webenginehelpviewer.h
)

extend_qtc_plugin(Help
  CONDITION TARGET qlitehtml
  FEATURE_INFO "litehtml help viewer"
  DEPENDS qlitehtml
  DEFINES QTC_LITEHTML_HELPVIEWER
  SOURCES
    litehtmlhelpviewer.cpp
    litehtmlhelpviewer.h
)
