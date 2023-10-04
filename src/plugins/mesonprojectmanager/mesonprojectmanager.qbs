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
        Depends { name: "CppEditor" }
        Depends { name: "ProjectExplorer" }
        Depends { name: "QtSupport" }

        cpp.includePaths: "."

        files: [
            "mesontools.cpp",
            "mesontools.h",
            "mesonwrapper.cpp",
            "mesonwrapper.h",
            "ninjawrapper.h",
            "toolwrapper.cpp",
            "toolwrapper.h",
            "kitdata.h",
            "mesonactionsmanager.cpp",
            "mesonactionsmanager.h",
            "buildoptions.h",
            "mesoninfo.h",
            "mesoninfoparser.h",
            "buildoptionsparser.h",
            "buildsystemfilesparser.h",
            "common.h",
            "infoparser.h",
            "targetparser.h",
            "target.h",
            "mesonpluginconstants.h",
            "mesonprojectplugin.cpp",
            "arrayoptionlineedit.cpp",
            "arrayoptionlineedit.h",
            "buildoptionsmodel.cpp",
            "buildoptionsmodel.h",
            "mesonbuildconfiguration.cpp",
            "mesonbuildconfiguration.h",
            "mesonbuildsystem.cpp",
            "mesonbuildsystem.h",
            "mesonproject.cpp",
            "mesonproject.h",
            "mesonprojectimporter.cpp",
            "mesonprojectimporter.h",
            "mesonprojectmanagertr.h",
            "mesonprojectparser.cpp",
            "mesonprojectparser.h",
            "mesonrunconfiguration.cpp",
            "mesonrunconfiguration.h",
            "ninjabuildstep.cpp",
            "ninjabuildstep.h",
            "mesonoutputparser.cpp",
            "mesonoutputparser.h",
            "ninjaparser.cpp",
            "ninjaparser.h",
            "mesonprojectnodes.cpp",
            "mesonprojectnodes.h",
            "projecttree.cpp",
            "projecttree.h",
            "resources_meson.qrc",
            "settings.cpp",
            "settings.h",
            "mesontoolkitaspect.cpp",
            "mesontoolkitaspect.h",
            "ninjatoolkitaspect.cpp",
            "ninjatoolkitaspect.h",
            "toolkitaspectwidget.cpp",
            "toolkitaspectwidget.h",
            "toolitemsettings.cpp",
            "toolitemsettings.h",
            "toolsmodel.cpp",
            "toolsmodel.h",
            "toolssettingsaccessor.cpp",
            "toolssettingsaccessor.h",
            "toolssettingspage.cpp",
            "toolssettingspage.h",
            "tooltreeitem.cpp",
            "tooltreeitem.h",
            "versionhelper.h",
        ]
    }

    QtcAutotest {
        name: "mesonwrapper"

        Depends { name: "Core" }
        Depends { name: "Utils" }

        cpp.defines: base.concat(project.testDefines)
        cpp.includePaths: "."

        files: [
            "mesonwrapper.cpp",
            "mesonwrapper.h",
            "ninjawrapper.h",
            "toolwrapper.h",
            "toolwrapper.cpp",
            "mesontools.h",
            "tests/testmesonwrapper.cpp",
        ]
    }

    QtcAutotest {
        name: "mesoninfoparser"

        Depends { name: "Core" }
        Depends { name: "Utils" }

        cpp.defines: base.concat(project.testDefines)
        cpp.includePaths: "."

        files: [
            "mesonwrapper.cpp",
            "mesonwrapper.h",
            "ninjawrapper.h",
            "toolwrapper.h",
            "toolwrapper.cpp",
            "mesontools.h",
            "mesoninfoparser.h",
            "tests/testmesoninfoparser.cpp",
        ]
    }

    QtcAutotest {
        name: "ninjaparser"

        Depends { name: "Core" }
        Depends { name: "ProjectExplorer" }
        Depends { name: "Utils" }

        cpp.includePaths: "."

        files: [
            "ninjaparser.cpp",
            "ninjaparser.h",
            "mesoninfoparser.h",
            "tests/testninjaparser.cpp",
        ]
    }

    QtcAutotest {
        name: "mesonparser"

        Depends { name: "Core" }
        Depends { name: "ProjectExplorer" }
        Depends { name: "Utils" }

        cpp.defines: base.concat("MESONPARSER_DISABLE_TASKS_FOR_TESTS")
        cpp.includePaths: "."

        files: [
            "mesonoutputparser.h",
            "mesonoutputparser.cpp",
            "tests/testmesonparser.cpp",
        ]
    }
}
