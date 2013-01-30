import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "QbsProjectManager"

    property var qbs_source_dir: qbs.getenv("QBS_SOURCE_DIR")
    property var qbs_build_dir: qbs.getenv("QBS_BUILD_DIR")

    condition: qbs_source_dir !== undefined && qbs_build_dir !== undefined

    Depends { name: "Qt"; submodules: [ "widgets", "script" ] }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "ExtensionSystem" }
    Depends { name: "TextEditor" }
    Depends { name: "QtSupport" }
    Depends { name: "QmlJS" }

    Depends { name: "cpp" }

    cpp.includePaths: base.concat([
        qbs_source_dir + "/src",
        qbs_source_dir + "/src/lib",
    ])

    cpp.defines: base.concat([
        'QBS_SOURCE_DIR="' + qbs_source_dir + '"',
        'QBS_BUILD_DIR="' + qbs_build_dir +'"',
        'QML_BUILD_STATIC_LIB'
    ])

    cpp.staticLibraries: {
        if (qbs.targetOS === "windows") {
            if (qbs.enableDebugCode) {
                return qbs_build_dir + "/lib/qbscored.lib"
            } else {
                return qbs_build_dir + "/lib/qbscore.lib"
            }
        } else {
            return qbs_build_dir + "/lib/libqbscore.a"
        }
    }

    cpp.dynamicLibraries: {
        if (qbs.targetOS === "windows") {
            return "shell32"
        }
    }

    files: [
        "qbsbuildconfiguration.cpp",
        "qbsbuildconfiguration.h",
        "qbsbuildconfigurationwidget.cpp",
        "qbsbuildconfigurationwidget.h",
        "qbsbuildstep.cpp",
        "qbsbuildstep.h",
        "qbsbuildstepconfigwidget.ui",
        "qbscleanstep.cpp",
        "qbscleanstep.h",
        "qbscleanstepconfigwidget.ui",
        "qbslogsink.cpp",
        "qbslogsink.h",
        "qbsnodes.cpp",
        "qbsnodes.h",
        "qbsparser.cpp",
        "qbsparser.h",
        "qbsproject.cpp",
        "qbsproject.h",
        "qbsprojectfile.cpp",
        "qbsprojectfile.h",
        "qbsprojectmanager.cpp",
        "qbsprojectmanager.h",
        "qbsprojectmanager_global.h",
        "qbsprojectmanagerconstants.h",
        "qbsprojectmanagerplugin.cpp",
        "qbsprojectmanagerplugin.h",
    ]
}

