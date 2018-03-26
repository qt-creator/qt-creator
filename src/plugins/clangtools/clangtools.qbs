import qbs

QtcPlugin {
    name: "ClangTools"

    Depends { name: "Debugger" }
    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "ExtensionSystem" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtcSsh" }
    Depends { name: "Utils" }

    Depends { name: "Qt.widgets" }

    pluginTestDepends: [
        "QbsProjectManager",
        "QmakeProjectManager",
    ]

    files: [
        "clangstaticanalyzer_global.h",
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
        "clangstaticanalyzerlogfilereader.cpp",
        "clangstaticanalyzerlogfilereader.h",
        "clangstaticanalyzerprojectsettings.cpp",
        "clangstaticanalyzerprojectsettings.h",
        "clangstaticanalyzerprojectsettingsmanager.cpp",
        "clangstaticanalyzerprojectsettingsmanager.h",
        "clangstaticanalyzerprojectsettingswidget.cpp",
        "clangstaticanalyzerprojectsettingswidget.h",
        "clangstaticanalyzerprojectsettingswidget.ui",
        "clangstaticanalyzerruncontrol.cpp",
        "clangstaticanalyzerruncontrol.h",
        "clangstaticanalyzerrunner.cpp",
        "clangstaticanalyzerrunner.h",
        "clangstaticanalyzersettings.cpp",
        "clangstaticanalyzersettings.h",
        "clangstaticanalyzertool.cpp",
        "clangstaticanalyzertool.h",
        "clangstaticanalyzerutils.cpp",
        "clangstaticanalyzerutils.h",
        "clangtoolsplugin.cpp",
        "clangtoolsplugin.h",
    ]

    Group {
        name: "Unit tests"
        condition: qtc.testsEnabled
        files: [
            "clangstaticanalyzerpreconfiguredsessiontests.cpp",
            "clangstaticanalyzerpreconfiguredsessiontests.h",
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

    Group {
        name: "Other files"
        fileTags: []
        files: [
            project.ide_source_tree + "/doc/src/analyze/creator-clang-static-analyzer.qdoc",
        ]
    }
}
