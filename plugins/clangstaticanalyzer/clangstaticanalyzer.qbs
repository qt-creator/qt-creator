import qbs

QtcCommercialPlugin {
    name: "ClangStaticAnalyzer"

    Depends { name: "AnalyzerBase" }
    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "ExtensionSystem" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtcSsh" } // TODO: export + recursive dependencies broken in qbs
    Depends { name: "Utils" }

    Depends { name: "Qt.widgets" }
    Depends { name: "Qt.network" }  // TODO: See above

    pluginTestDepends: [
        "QbsProjectManager",
        "QmakeProjectManager",
    ]

    files: [
        "clangstaticanalyzerconfigwidget.cpp",
        "clangstaticanalyzerconfigwidget.h",
        "clangstaticanalyzerconfigwidget.ui",
        "clangstaticanalyzerconstants.h",
        "clangstaticanalyzerdiagnostic.cpp",
        "clangstaticanalyzerdiagnostic.h",
        "clangstaticanalyzerdiagnosticmodel.cpp",
        "clangstaticanalyzerdiagnosticmodel.h",
        "clangstaticanalyzerdiagnosticview.cpp",
        "clangstaticanalyzerdiagnosticview.h",
        "clangstaticanalyzerlicensecheck.h",
        "clangstaticanalyzerlogfilereader.cpp",
        "clangstaticanalyzerlogfilereader.h",
        "clangstaticanalyzerplugin.cpp",
        "clangstaticanalyzerplugin.h",
        "clangstaticanalyzerprojectsettings.cpp",
        "clangstaticanalyzerprojectsettings.h",
        "clangstaticanalyzerprojectsettingsmanager.cpp",
        "clangstaticanalyzerprojectsettingsmanager.h",
        "clangstaticanalyzerprojectsettingswidget.cpp",
        "clangstaticanalyzerprojectsettingswidget.h",
        "clangstaticanalyzerprojectsettingswidget.ui",
        "clangstaticanalyzerruncontrol.cpp",
        "clangstaticanalyzerruncontrol.h",
        "clangstaticanalyzerruncontrolfactory.cpp",
        "clangstaticanalyzerruncontrolfactory.h",
        "clangstaticanalyzerrunner.cpp",
        "clangstaticanalyzerrunner.h",
        "clangstaticanalyzersettings.cpp",
        "clangstaticanalyzersettings.h",
        "clangstaticanalyzertool.cpp",
        "clangstaticanalyzertool.h",
        "clangstaticanalyzerutils.cpp",
        "clangstaticanalyzerutils.h",
        "clangstaticanalyzer_global.h",
    ]

    Group {
        name: "Unit tests"
        condition: project.testsEnabled
        files: [
            "clangstaticanalyzerunittests.cpp",
            "clangstaticanalyzerunittests.h",
            "clangstaticanalyzerunittests.qrc",
        ]
    }

    Group {
        name: "Unit test resources"
        prefix: "unit-tests/"
        fileTags: []
        files: ["**/*"]
    }
}
