import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "AnalyzerBase"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "RemoteLinux" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }

    Depends { name: "cpp" }
    cpp.defines: [
        "ANALYZER_LIBRARY",
        "QT_NO_CAST_FROM_ASCII"
    ]
    cpp.includePaths: [
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "analyzerbase.qrc",
        "analyzerbase_global.h",
        "analyzerconstants.h",
        "analyzeroptionspage.cpp",
        "analyzeroptionspage.h",
        "analyzerplugin.cpp",
        "analyzerplugin.h",
        "analyzerrunconfigwidget.cpp",
        "analyzerrunconfigwidget.h",
        "analyzerruncontrol.h",
        "analyzersettings.cpp",
        "analyzersettings.h",
        "analyzerstartparameters.h",
        "analyzerutils.cpp",
        "analyzerutils.h",
        "ianalyzerengine.cpp",
        "ianalyzerengine.h",
        "ianalyzertool.cpp",
        "startremotedialog.cpp",
        "startremotedialog.h",
        "analyzermanager.cpp",
        "analyzermanager.h",
        "analyzerruncontrol.cpp",
        "analyzerruncontrolfactory.cpp",
        "analyzerruncontrolfactory.h",
        "ianalyzertool.h",
        "images/analyzer_category.png",
        "images/analyzer_mode.png",
        "images/analyzer_start_small.png"
    ]

    ProductModule {
        Depends { name: "cpp" }
        cpp.includePaths: ["."]

        Depends { name: "CPlusPlus" }
    }
}

