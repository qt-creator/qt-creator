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

set(HELPVIEWER_DEFAULT_BACKEND "litehtml" CACHE STRING "Sets default help viewer backend")
set_property(CACHE HELPVIEWER_DEFAULT_BACKEND PROPERTY STRINGS "litehtml;qtwebengine;textbrowser")

extend_qtc_plugin(Help
  CONDITION HELPVIEWER_DEFAULT_BACKEND
  DEFINES QTC_DEFAULT_HELPVIEWER_BACKEND="${HELPVIEWER_DEFAULT_BACKEND}"
)

extend_qtc_plugin(Help
  CONDITION FWWebKit AND FWAppKit AND Qt5_VERSION VERSION_LESS 6.0.0
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

extend_qtc_plugin(Help
  CONDITION TARGET qlitehtml
  FEATURE_INFO "litehtml help viewer"
  DEPENDS qlitehtml
  DEFINES QTC_LITEHTML_HELPVIEWER
  SOURCES
    litehtmlhelpviewer.cpp
    litehtmlhelpviewer.h
)
