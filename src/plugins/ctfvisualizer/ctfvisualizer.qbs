import qbs.Utilities

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

    Properties {
        condition: qbs.toolchain.contains("gcc") && (!qbs.toolchain.contains("clang")
            || Utilities.versionCompare(cpp.compilerVersion, "17") >= 0)
        cpp.cxxFlags: "-Wno-deprecated-literal-operator"
    }

    files: [
        "ctfstatisticsmodel.cpp", "ctfstatisticsmodel.h",
        "ctfstatisticsview.cpp", "ctfstatisticsview.h",
        "ctftimelinemodel.cpp", "ctftimelinemodel.h",
        "ctftracemanager.cpp", "ctftracemanager.h",
        "ctfvisualizerconstants.h",
        "ctfvisualizerplugin.cpp",
        "ctfvisualizertool.cpp", "ctfvisualizertool.h",
        "ctfvisualizertraceview.cpp", "ctfvisualizertraceview.h",
        "ctfvisualizertr.h",
        "../../libs/3rdparty/json/json.hpp",
    ]
}
