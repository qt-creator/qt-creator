import qbs.FileInfo

Project {
    name: "MesonProjectManager"

    property stringList testDefines: [
        'MESON_SAMPLES_DIR="' + FileInfo.joinPaths(path, "tests", "resources") + '"',
    ]

    QtcPlugin {
        Depends { name: "Qt.widgets" }
        Depends { name: "Utils" }

        Depends { name: "Core" }
        Depends { name: "CppTools" }
        Depends { name: "ProjectExplorer" }
        Depends { name: "QtSupport" }
        Depends { name: "app_version_header" }

        pluginRecommends: [
            "Designer"
        ]

        cpp.includePaths: "."

        files: [
            "exewrappers/mesontools.cpp",
            "exewrappers/mesontools.h",
            "exewrappers/mesonwrapper.cpp",
            "exewrappers/mesonwrapper.h",
            "exewrappers/ninjawrapper.h",
            "exewrappers/toolwrapper.cpp",
            "exewrappers/toolwrapper.h",
            "kithelper/kitdata.h",
            "kithelper/kithelper.h",
            "machinefiles/machinefilemanager.cpp",
            "machinefiles/machinefilemanager.h",
            "machinefiles/nativefilegenerator.cpp",
            "machinefiles/nativefilegenerator.h",
            "mesonactionsmanager/mesonactionsmanager.cpp",
            "mesonactionsmanager/mesonactionsmanager.h",
            "mesoninfoparser/buildoptions.h",
            "mesoninfoparser/mesoninfo.h",
            "mesoninfoparser/mesoninfoparser.h",
            "mesoninfoparser/parsers/buildoptionsparser.h",
            "mesoninfoparser/parsers/buildsystemfilesparser.h",
            "mesoninfoparser/parsers/common.h",
            "mesoninfoparser/parsers/infoparser.h",
            "mesoninfoparser/parsers/targetparser.h",
            "mesoninfoparser/target.h",
            "mesonpluginconstants.h",
            "mesonprojectplugin.cpp",
            "mesonprojectplugin.h",
            "project/buildoptions/mesonbuildsettingswidget.cpp",
            "project/buildoptions/mesonbuildsettingswidget.h",
            "project/buildoptions/mesonbuildsettingswidget.ui",
            "project/buildoptions/mesonbuildstepconfigwidget.cpp",
            "project/buildoptions/mesonbuildstepconfigwidget.h",
            "project/buildoptions/mesonbuildstepconfigwidget.ui",
            "project/buildoptions/optionsmodel/arrayoptionlineedit.cpp",
            "project/buildoptions/optionsmodel/arrayoptionlineedit.h",
            "project/buildoptions/optionsmodel/buildoptionsmodel.cpp",
            "project/buildoptions/optionsmodel/buildoptionsmodel.h",
            "project/mesonbuildconfiguration.cpp",
            "project/mesonbuildconfiguration.h",
            "project/mesonbuildsystem.cpp",
            "project/mesonbuildsystem.h",
            "project/mesonprocess.cpp",
            "project/mesonprocess.h",
            "project/mesonproject.cpp",
            "project/mesonproject.h",
            "project/mesonprojectimporter.cpp",
            "project/mesonprojectimporter.h",
            "project/mesonprojectparser.cpp",
            "project/mesonprojectparser.h",
            "project/mesonrunconfiguration.cpp",
            "project/mesonrunconfiguration.h",
            "project/ninjabuildstep.cpp",
            "project/ninjabuildstep.h",
            "project/outputparsers/mesonoutputparser.cpp",
            "project/outputparsers/mesonoutputparser.h",
            "project/outputparsers/ninjaparser.cpp",
            "project/outputparsers/ninjaparser.h",
            "project/projecttree/mesonprojectnodes.cpp",
            "project/projecttree/mesonprojectnodes.h",
            "project/projecttree/projecttree.cpp",
            "project/projecttree/projecttree.h",
            "settings/general/generalsettingspage.cpp",
            "settings/general/generalsettingspage.h",
            "settings/general/generalsettingswidget.cpp",
            "settings/general/generalsettingswidget.h",
            "settings/general/generalsettingswidget.ui",
            "settings/general/settings.cpp",
            "settings/general/settings.h",
            "settings/tools/kitaspect/mesontoolkitaspect.cpp",
            "settings/tools/kitaspect/mesontoolkitaspect.h",
            "settings/tools/kitaspect/ninjatoolkitaspect.cpp",
            "settings/tools/kitaspect/ninjatoolkitaspect.h",
            "settings/tools/kitaspect/toolkitaspectwidget.cpp",
            "settings/tools/kitaspect/toolkitaspectwidget.h",
            "settings/tools/toolitemsettings.cpp",
            "settings/tools/toolitemsettings.h",
            "settings/tools/toolitemsettings.ui",
            "settings/tools/toolsmodel.cpp",
            "settings/tools/toolsmodel.h",
            "settings/tools/toolssettingsaccessor.cpp",
            "settings/tools/toolssettingsaccessor.h",
            "settings/tools/toolssettingspage.cpp",
            "settings/tools/toolssettingspage.h",
            "settings/tools/toolssettingswidget.cpp",
            "settings/tools/toolssettingswidget.h",
            "settings/tools/toolssettingswidget.ui",
            "settings/tools/tooltreeitem.cpp",
            "settings/tools/tooltreeitem.h",
            "versionhelper.h",
        ]
    }

    QtcAutotest {
        name: "tst_mesonwrapper"
        condition: project.withAutotests

        Depends { name: "Core" }
        Depends { name: "Utils" }

        cpp.defines: project.testDefines
        cpp.includePaths: "."

        files: [
            "exewrappers/mesonwrapper.cpp",
            "exewrappers/mesonwrapper.h",
            "exewrappers/ninjawrapper.h",
            "exewrappers/toolwrapper.h",
            "exewrappers/toolwrapper.cpp",
            "exewrappers/mesontools.h",
            "tests/testmesonwrapper.cpp",
        ]
    }

    QtcAutotest {
        name: "tst_mesoninfoparser"
        condition: project.withAutotests

        Depends { name: "Core" }
        Depends { name: "Utils" }

        cpp.defines: project.testDefines
        cpp.includePaths: "."

        files: [
            "exewrappers/mesonwrapper.cpp",
            "exewrappers/mesonwrapper.h",
            "exewrappers/ninjawrapper.h",
            "exewrappers/toolwrapper.h",
            "exewrappers/toolwrapper.cpp",
            "exewrappers/mesontools.h",
            "mesoninfoparser/mesoninfoparser.h",
            "tests/testmesoninfoparser.cpp",
        ]
    }

    QtcAutotest {
        name: "tst_ninjaparser"
        condition: project.withAutotests

        Depends { name: "Core" }
        Depends { name: "ProjectExplorer" }
        Depends { name: "Utils" }

        cpp.includePaths: "."

        files: [
            "project/outputparsers/ninjaparser.cpp",
            "project/outputparsers/ninjaparser.h",
            "mesoninfoparser/mesoninfoparser.h",
            "tests/testninjaparser.cpp",
        ]
    }

    QtcAutotest {
        name: "tst_mesonparser"
        condition: project.withAutotests

        Depends { name: "Core" }
        Depends { name: "ProjectExplorer" }
        Depends { name: "Utils" }

        cpp.defines: "MESONPARSER_DISABLE_TASKS_FOR_TESTS"
        cpp.includePaths: "."

        files: [
            "project/outputparsers/mesonoutputparser.h",
            "project/outputparsers/mesonoutputparser.cpp",
            "tests/testmesonparser.cpp",
        ]
    }
}
