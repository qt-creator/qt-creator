import qbs

QtcProduct {
    type: ["application"]
    name: project.ide_app_target
    consoleApplication: qbs.debugInformation

    cpp.rpaths: qbs.targetOS.contains("osx") ? ["@executable_path/.."]
                                             : ["$ORIGIN/../" + project.libDirName + "/qtcreator"]
    cpp.includePaths: [
        project.sharedSourcesDir + "/qtsingleapplication",
        project.sharedSourcesDir + "/qtlockedfile",
    ]

    Depends { name: "app_version_header" }
    Depends { name: "Qt"; submodules: ["widgets", "network"] }
    Depends { name: "Utils" }
    Depends { name: "ExtensionSystem" }

    files: [
        "main.cpp",
        "qtcreator.rc",
        "../shared/qtsingleapplication/qtsingleapplication.h",
        "../shared/qtsingleapplication/qtsingleapplication.cpp",
        "../shared/qtsingleapplication/qtlocalpeer.h",
        "../shared/qtsingleapplication/qtlocalpeer.cpp",
        "../shared/qtlockedfile/qtlockedfile.cpp",
        "../tools/qtcreatorcrashhandler/crashhandlersetup.cpp",
        "../tools/qtcreatorcrashhandler/crashhandlersetup.h"
    ]

    Group {
        name: "qtcreator.sh"
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("osx")
        files: "../../bin/qtcreator.sh"
        qbs.install: true
        qbs.installDir: "bin"
    }

    Group {
        name: "QtLockedFile_unix"
        condition: qbs.targetOS.contains("unix")
        files: [
            "../shared/qtlockedfile/qtlockedfile_unix.cpp"
        ]
    }

    Group {
        name: "QtLockedFile_win"
        condition: qbs.targetOS.contains("windows")
        files: [
            "../shared/qtlockedfile/qtlockedfile_win.cpp"
        ]
    }

    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: project.ide_bin_path
    }
}
