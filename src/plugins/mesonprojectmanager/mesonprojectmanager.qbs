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
        Depends { name: "ProjectExplorer" }
        Depends { name: "QtSupport" }

        cpp.includePaths: "."

        files: [
            "mesontools.cpp",
            "mesontools.h",
            "kitdata.h",
            "mesonactionsmanager.cpp",
            "mesonactionsmanager.h",
            "buildoptions.h",
            "mesoninfoparser.h",
            "buildoptionsparser.h",
            "common.h",
            "infoparser.h",
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
            "toolkitaspectwidget.cpp",
            "toolkitaspectwidget.h",
            "toolsmodel.cpp",
            "toolsmodel.h",
            "toolssettingsaccessor.cpp",
            "toolssettingsaccessor.h",
            "toolssettingspage.cpp",
            "toolssettingspage.h",
        ]
    }

    QtcAutotest {
        name: "mesonwrapper"

        Depends { name: "Core" }
        Depends { name: "Utils" }

        cpp.defines: base.concat(project.testDefines)
        cpp.includePaths: "."

        files: [
            "mesontools.h",
            "mesontools.cpp",
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
            "mesontools.h",
            "mesontools.cpp",
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
