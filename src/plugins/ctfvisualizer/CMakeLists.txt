add_qtc_plugin(CtfVisualizer
  DEPENDS Tracing Qt5::QuickWidgets
  INCLUDES ${PROJECT_SOURCE_DIR}/src
  PLUGIN_DEPENDS Core Debugger ProjectExplorer
  SOURCES
    ctfstatisticsmodel.cpp
    ctfstatisticsview.cpp
    ctfvisualizerplugin.cpp
    ctfvisualizertool.cpp
    ctftimelinemodel.cpp
    ctftracemanager.cpp
    ctfvisualizertraceview.cpp
    ctfstatisticsmodel.h
    ctfstatisticsview.h
    ctfvisualizerplugin.h
    ctfvisualizertool.h
    ctftimelinemodel.h
    ctftracemanager.h
    ctfvisualizerconstants.h
    ctfvisualizertraceview.h
    ../../libs/3rdparty/json/json.hpp
)
