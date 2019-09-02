import qbs

QtcPlugin {
    name: "CtfVisualizer"

    Depends { name: "Core" }
    Depends { name: "Debugger" }
    Depends { name: "Tracing" }
    Depends { name: "Utils" }

    Depends {
        name: "Qt"
        submodules: [ "quick", "quickwidgets" ]
    }

    files: [
        "ctfstatisticsmodel.cpp", "ctfstatisticsmodel.h",
        "ctfstatisticsview.cpp", "ctfstatisticsview.h",
        "ctftimelinemodel.cpp", "ctftimelinemodel.h",
        "ctftracemanager.cpp", "ctftracemanager.h",
        "ctfvisualizerplugin.cpp", "ctfvisualizerplugin.h",
        "ctfvisualizertool.cpp", "ctfvisualizertool.h",
        "ctfvisualizertraceview.cpp", "ctfvisualizertraceview.h",
        "ctfvisualizerconstants.h",
        "../../libs/3rdparty/json/json.hpp",
    ]
}
