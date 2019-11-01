add_qtc_library(ExtensionSystem
  DEPENDS Aggregation Utils
  PUBLIC_DEPENDS Qt5::Core Qt5::Widgets
  SOURCES
    extensionsystem_global.h
    invoker.cpp invoker.h
    iplugin.cpp iplugin.h iplugin_p.h
    optionsparser.cpp optionsparser.h
    plugindetailsview.cpp plugindetailsview.h plugindetailsview.ui
    pluginerroroverview.cpp pluginerroroverview.h pluginerroroverview.ui
    pluginerrorview.cpp pluginerrorview.h pluginerrorview.ui
    pluginmanager.cpp pluginmanager.h pluginmanager_p.h
    pluginspec.cpp pluginspec.h pluginspec_p.h
    pluginview.cpp pluginview.h
  EXPLICIT_MOC
    pluginmanager.h
    pluginmanager_p.h
  SKIP_AUTOMOC pluginmanager.cpp
)
