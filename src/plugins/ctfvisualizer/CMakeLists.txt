add_qtc_plugin(CtfVisualizer
  CONDITION TARGET Tracing
  DEPENDS Tracing Qt::QuickWidgets
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
    ctfvisualizertool.h
    ctfvisualizertr.h
    ctftimelinemodel.h
    ctftracemanager.h
    ctfvisualizerconstants.h
    ctfvisualizertraceview.h
    ../../libs/3rdparty/json/json.hpp
  SBOM_ARGS
    USE_ATTRIBUTION_FILES
    CREATE_SBOM_FOR_EACH_ATTRIBUTION
    ATTRIBUTION_FILE_PATHS
      "${QtCreator_SOURCE_DIR}/qt_attributions.json"
    ATTRIBUTION_IDS
      json-nlohmann
)
