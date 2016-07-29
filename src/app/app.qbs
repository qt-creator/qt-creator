import qbs

QtcProduct {
    Depends { name: "bundle" }
    Depends { name: "ib"; condition: qbs.targetOS.contains("macos") }

    Properties {
        condition: qbs.targetOS.contains("macos")
        ib.appIconName: "qtcreator"
    }

    Properties {
        condition: qbs.targetOS.contains("windows")
        consoleApplication: qbs.debugInformation
    }
    consoleApplication: false

    type: ["application"]
    name: "qtcreator"
    targetName: qtc.ide_app_target
    version: qtc.qtcreator_version

    installDir: qtc.ide_bin_path
    property bool qtcRunnable: true

    cpp.rpaths: qbs.targetOS.contains("macos") ? ["@executable_path/../Frameworks"]
                                             : ["$ORIGIN/../" + qtc.libDirName + "/qtcreator"]
    cpp.includePaths: [
        project.sharedSourcesDir + "/qtsingleapplication",
        project.sharedSourcesDir + "/qtlockedfile",
    ]

    Depends { name: "app_version_header" }
    Depends { name: "Qt"; submodules: ["widgets", "network"] }
    Depends { name: "Utils" }
    Depends { name: "ExtensionSystem" }

    files: [
        "Info.plist",
        "main.cpp",
        "qtcreator.xcassets",
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
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("macos")
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
        condition: qbs.targetOS.contains("macos")
        fileTagsFilter: ["aggregate_infoplist", "pkginfo", "compiled_assetcatalog"]
        qbs.install: true
        qbs.installSourceBase: product.buildDirectory
    }
}
