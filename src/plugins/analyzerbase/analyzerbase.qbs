import qbs 1.0

QtcPlugin {
    name: "AnalyzerBase"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }
    Depends { name: "QtcSsh" }

    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }

    files: [
        "analyzerbase.qrc",
        "analyzerbase_global.h",
        "analyzerconstants.h",
        "analyzermanager.cpp",
        "analyzermanager.h",
        "analyzerplugin.cpp",
        "analyzerplugin.h",
        "analyzerrunconfigwidget.cpp",
        "analyzerrunconfigwidget.h",
        "analyzerruncontrol.cpp",
        "analyzerruncontrol.h",
        "analyzerstartparameters.h",
        "analyzerutils.cpp",
        "analyzerutils.h",
        "detailederrorview.cpp",
        "detailederrorview.h",
        "ianalyzertool.cpp",
        "ianalyzertool.h",
        "startremotedialog.cpp",
        "startremotedialog.h",
        "images/analyzer_category.png",
        "images/analyzer_mode.png",
        "images/analyzer_start_small.png",
    ]

    Export {
        Depends { name: "CPlusPlus" }
    }
}

