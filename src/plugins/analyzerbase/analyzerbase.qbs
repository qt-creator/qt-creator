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
    cpp.defines: base.concat("ANALYZER_LIBRARY")

    files: [
        "analyzerbase.qrc",
        "analyzerbase_global.h",
        "analyzerconstants.h",
        "analyzermanager.cpp",
        "analyzermanager.h",
        "analyzeroptionspage.cpp",
        "analyzeroptionspage.h",
        "analyzerplugin.cpp",
        "analyzerplugin.h",
        "analyzerrunconfigwidget.cpp",
        "analyzerrunconfigwidget.h",
        "analyzerruncontrol.cpp",
        "analyzerruncontrol.h",
        "analyzerruncontrolfactory.cpp",
        "analyzerruncontrolfactory.h",
        "analyzersettings.cpp",
        "analyzersettings.h",
        "analyzerstartparameters.h",
        "analyzerutils.cpp",
        "analyzerutils.h",
        "ianalyzerengine.cpp",
        "ianalyzerengine.h",
        "ianalyzertool.cpp",
        "ianalyzertool.h",
        "startremotedialog.cpp",
        "startremotedialog.h",
        "images/analyzer_category.png",
        "images/analyzer_mode.png",
        "images/analyzer_start_small.png",
    ]

    ProductModule {
        Depends { name: "CPlusPlus" }
    }
}

