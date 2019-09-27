import qbs
import qbs.FileInfo

QtcPlugin {
    name: "ClangTools"

    Depends { name: "Debugger" }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "CppTools" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtcSsh" }
    Depends { name: "Utils" }

    Depends { name: "libclang"; required: false }
    Depends { name: "yaml-cpp" }
    Depends { name: "clang_defines" }

    Depends { name: "Qt.widgets" }

    pluginTestDepends: [
        "QbsProjectManager",
        "QmakeProjectManager",
    ]

    condition: libclang.present

    cpp.includePaths: base.concat(libclang.llvmIncludeDir)
    cpp.libraryPaths: base.concat(libclang.llvmLibDir)
    cpp.dynamicLibraries: base.concat(libclang.llvmLibs)
    cpp.rpaths: base.concat(libclang.llvmLibDir)

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
        "clangtoolsprojectsettings.cpp",
        "clangtoolsprojectsettings.h",
        "clangtoolsprojectsettingswidget.cpp",
        "clangtoolsprojectsettingswidget.h",
        "clangtoolsprojectsettingswidget.ui",
        "clangtoolssettings.cpp",
        "clangtoolssettings.h",
        "clangtoolsutils.cpp",
        "clangtoolsutils.h",
        "clangtoolsplugin.cpp",
        "clangtoolsplugin.h",
        "runsettingswidget.cpp",
        "runsettingswidget.h",
        "runsettingswidget.ui",
        "settingswidget.cpp",
        "settingswidget.h",
        "settingswidget.ui",
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
            project.ide_source_tree + "/doc/src/analyze/creator-clang-static-analyzer.qdoc",
        ]
    }
}
