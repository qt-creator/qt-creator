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
    Depends { name: "libclang"; required: false }

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
        "clangstaticanalyzerconfigwidget.cpp",
        "clangstaticanalyzerconfigwidget.h",
        "clangstaticanalyzerconfigwidget.ui",
        "clangstaticanalyzerdiagnosticview.cpp",
        "clangstaticanalyzerdiagnosticview.h",
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
        "clangstaticanalyzertool.cpp",
        "clangstaticanalyzertool.h",
        "clangtidyclazyruncontrol.cpp",
        "clangtidyclazyruncontrol.h",
        "clangtidyclazyrunner.cpp",
        "clangtidyclazyrunner.h",
        "clangtidyclazytool.cpp",
        "clangtidyclazytool.h",
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
        "clangtoolslogfilereader.cpp",
        "clangtoolslogfilereader.h",
        "clangtoolssettings.cpp",
        "clangtoolssettings.h",
        "clangtoolsutils.cpp",
        "clangtoolsutils.h",
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
