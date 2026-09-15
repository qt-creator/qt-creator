Project {
    QtcPlugin {
        name: "CMakeProjectManager"

        Depends { name: "Qt.widgets" }
        Depends { name: "CMakeLang" }
        Depends { name: "RstLang" }
        Depends { name: "McpServerLib" }
        Depends { name: "Utils" }

        Depends { name: "Core" }
        Depends { name: "CppEditor" }
        Depends { name: "Debugger" }
        Depends { name: "ProjectExplorer" }
        Depends { name: "TextEditor" }
        Depends { name: "QtSupport" }

        files: [
            "builddirparameters.cpp",
            "builddirparameters.h",
            "cmake_global.h",
            "cmakeabstractprocessstep.cpp",
            "cmakeabstractprocessstep.h",
            "cmakeautogenparser.cpp",
            "cmakeautogenparser.h",
            "cmakebuildconfiguration.cpp",
            "cmakebuildconfiguration.h",
            "cmakebuildstep.cpp",
            "cmakebuildstep.h",
            "cmakecodestyle.cpp",
            "cmakecodestyle.h",
            "cmakebuildsystem.cpp",
            "cmakebuildsystem.h",
            "cmakebuildtarget.h",
            "cmakecommandkeywords.cpp",
            "cmakecommandkeywords.h",
            "cmakeconfigitem.cpp",
            "cmakeconfigitem.h",
            "cmakeeditor.cpp",
            "cmakeeditor.h",
            "cmakefilecompletionassist.cpp",
            "cmakefilecompletionassist.h",
            "cmakeformatter.cpp",
            "cmakeformatter.h",
            "cmakeinstallrules.cpp",
            "cmakeinstallrules.h",
            "cmakeinstallstep.cpp",
            "cmakeinstallstep.h",
            "cmakekitaspect.h",
            "cmakekitaspect.cpp",
            "cmakelocatorfilter.cpp",
            "cmakelocatorfilter.h",
            "cmakeoutline.cpp",
            "cmakeoutline.h",
            "cmakeoutputparser.cpp",
            "cmakeoutputparser.h",
            "cmakeproject.cpp",
            "cmakeproject.h",
            "cmakeproject.qrc",
            "cmakeprojectimporter.cpp",
            "cmakeprojectimporter.h",
            "cmakeprojectconstants.h",
            "cmakeprojectmanager.cpp",
            "cmakeprojectmanager.h",
            "cmakeprojectmanagertr.h",
            "cmakeprojectnodes.cpp",
            "cmakeprojectnodes.h",
            "cmakeprojectplugin.cpp",
            "cmakequickfixes.cpp",
            "cmakequickfixes.h",
            "cmakesemantichighlighter.cpp",
            "cmakesemantichighlighter.h",
            "cmaketool.cpp",
            "cmaketool.h",
            "cmaketoolmanager.cpp",
            "cmaketoolmanager.h",
            "cmaketoolsettingsaccessor.cpp",
            "cmaketoolsettingsaccessor.h",
            "cmakeusages.cpp",
            "cmakeusages.h",
            "cmakeutils.cpp",
            "cmakeutils.h",
            "cmakesettingspage.h",
            "cmakesettingspage.cpp",
            "cmakeindenter.h",
            "cmakeindenter.cpp",
            "cmakeautocompleter.h",
            "cmakeautocompleter.cpp",
            "cmakespecificsettings.h",
            "cmakespecificsettings.cpp",
            "conditionalsources.cpp",
            "conditionalsources.h",
            "configmodel.cpp",
            "configmodel.h",
            "configmodelitemdelegate.cpp",
            "configmodelitemdelegate.h",
            "fileapidataextractor.cpp",
            "fileapidataextractor.h",
            "fileapiparser.cpp",
            "fileapiparser.h",
            "fileapireader.cpp",
            "fileapireader.h",
            "headerdependencies.cpp",
            "headerdependencies.h",
            "headerdependencyscanner.cpp",
            "headerdependencyscanner.h",
            "headerdependencyupdater.cpp",
            "headerdependencyupdater.h",
            "mcptools.cpp",
            "mcptools.h",
            "presetsparser.cpp",
            "presetsparser.h",
            "presetsmacros.cpp",
            "presetsmacros.h",
            "projecttreehelper.cpp",
            "projecttreehelper.h",
            "qtinstallerpackages.cpp",
            "qtinstallerpackages.h",
            "targethelper.cpp",
            "targethelper.h",
            "testpresetshelper.cpp",
            "testpresetshelper.h",
        ]

        QtcTestResources { files: "testcases/**/*" }
    }

    QtcAutotest {
        name: "CMake install rules test"
        Depends { name: "CMakeProjectManager" }
        Depends { name: "ProjectExplorer" }
        Depends { name: "Utils" }
        files: [
            "cmakeinstallrules.cpp",
            "tests/tst_cmake_install_rules.cpp",
        ]
    }

    QtcAutotest {
        name: "CMake presets test"
        Depends { name: "CMakeProjectManager" }
        Depends { name: "ProjectExplorer" }
        Depends { name: "Utils" }
        files: [
            "presetsparser.cpp",
            "presetsmacros.cpp",
            "testpresetshelper.cpp",
            "tests/tst_cmake_test_presets.cpp",
        ]
    }

    QtcAutotest {
        name: "CMake header dependencies test"
        Depends { name: "Utils" }
        files: [
            "headerdependencies.cpp",
            "tests/tst_headerdependencies.cpp",
        ]
    }

    QtcAutotest {
        name: "CMake header dependency scanner test"
        Depends { name: "ProjectExplorer" }
        Depends { name: "Utils" }
        files: [
            "headerdependencies.cpp",
            "headerdependencyscanner.cpp",
            "tests/tst_headerdependencyscanner.cpp",
        ]
    }

    QtcAutotest {
        name: "CMake header dependency updater test"
        Depends { name: "Core" }
        Depends { name: "CppEditor" }
        Depends { name: "ProjectExplorer" }
        Depends { name: "Utils" }
        files: [
            "headerdependencies.cpp",
            "headerdependencyscanner.cpp",
            "headerdependencyupdater.cpp",
            "tests/tst_headerdependencyupdater.cpp",
        ]
    }
}
