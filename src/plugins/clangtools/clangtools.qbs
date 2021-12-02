import qbs
import qbs.FileInfo

QtcPlugin {
    name: "ClangTools"

    Depends { name: "Core" }
    Depends { name: "CppEditor" }
    Depends { name: "Debugger" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport"; condition: qtc.testsEnabled }
    Depends { name: "QtcSsh" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }

    Depends { name: "yaml-cpp" }
    Depends { name: "clang_defines" }

    Depends { name: "Qt.widgets" }

    pluginTestDepends: [
        "QbsProjectManager",
        "QmakeProjectManager",
    ]

    files: [
        "clangfileinfo.h",
        "clangfixitsrefactoringchanges.cpp",
        "clangfixitsrefactoringchanges.h",
        "clangselectablefilesdialog.cpp",
        "clangselectablefilesdialog.h",
        "clangselectablefilesdialog.ui",
        "clangtidyclazyrunner.cpp",
        "clangtidyclazyrunner.h",
        "clangtool.cpp",
        "clangtool.h",
        "clangtoolruncontrol.cpp",
        "clangtoolruncontrol.h",
        "clangtoolrunner.cpp",
        "clangtoolrunner.h",
        "clangtools_global.h",
        "clangtoolsconstants.h",
        "clangtoolsdiagnostic.cpp",
        "clangtoolsdiagnostic.h",
        "clangtoolsdiagnosticmodel.cpp",
        "clangtoolsdiagnosticmodel.h",
        "clangtoolsdiagnosticview.cpp",
        "clangtoolsdiagnosticview.h",
        "clangtoolslogfilereader.cpp",
        "clangtoolslogfilereader.h",
        "clangtoolsplugin.cpp",
        "clangtoolsplugin.h",
        "clangtoolsprojectsettings.cpp",
        "clangtoolsprojectsettings.h",
        "clangtoolsprojectsettingswidget.cpp",
        "clangtoolsprojectsettingswidget.h",
        "clangtoolssettings.cpp",
        "clangtoolssettings.h",
        "clangtoolsutils.cpp",
        "clangtoolsutils.h",
        "clazychecks.ui",
        "diagnosticconfigswidget.cpp",
        "diagnosticconfigswidget.h",
        "diagnosticmark.cpp",
        "diagnosticmark.h",
        "documentclangtoolrunner.cpp",
        "documentclangtoolrunner.h",
        "documentquickfixfactory.cpp",
        "documentquickfixfactory.h",
        "executableinfo.cpp",
        "executableinfo.h",
        "filterdialog.cpp",
        "filterdialog.h",
        "filterdialog.ui",
        "runsettingswidget.cpp",
        "runsettingswidget.h",
        "runsettingswidget.ui",
        "settingswidget.cpp",
        "settingswidget.h",
        "settingswidget.ui",
        "tidychecks.ui",
        "virtualfilesystemoverlay.cpp",
        "virtualfilesystemoverlay.h",
    ]

    Group {
        name: "Unit tests"
        condition: qtc.testsEnabled
        files: [
            "clangtoolspreconfiguredsessiontests.cpp",
            "clangtoolspreconfiguredsessiontests.h",
            "clangtoolsunittests.cpp",
            "clangtoolsunittests.h",
            "clangtoolsunittests.qrc",
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
            project.ide_source_tree + "/doc/qtcreator/src/analyze/creator-clang-static-analyzer.qdoc",
        ]
    }
}
