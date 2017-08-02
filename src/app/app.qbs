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

    property bool isBundle: qbs.targetOS.contains("darwin") && bundle.isBundle
    installDir: isBundle ? qtc.ide_app_path : qtc.ide_bin_path
    installTags: isBundle ? ["bundle.content"] : base
    installSourceBase: isBundle ? buildDirectory : base
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
        "../shared/qtsingleapplication/qtsingleapplication.h",
        "../shared/qtsingleapplication/qtsingleapplication.cpp",
        "../shared/qtsingleapplication/qtlocalpeer.h",
        "../shared/qtsingleapplication/qtlocalpeer.cpp",
        "../shared/qtlockedfile/qtlockedfile.cpp",
        "../tools/qtcreatorcrashhandler/crashhandlersetup.cpp",
        "../tools/qtcreatorcrashhandler/crashhandlersetup.h"
    ]

    Group {
        // We need the version in two separate formats for the .rc file
        //  RC_VERSION=4,3,82,0 (quadruple)
        //  RC_VERSION_STRING="4.4.0-beta1" (free text)
        // Also, we need to replace space with \x20 to be able to work with both rc and windres
        cpp.defines: outer.concat(["RC_VERSION=" + qtc.qtcreator_version.replace(/\./g, ",") + ",0",
                                   "RC_VERSION_STRING=" + qtc.qtcreator_display_version,
                                   "RC_COPYRIGHT=2008-" + qtc.qtcreator_copyright_year
                                   + " The Qt Company Ltd".replace(/ /g, "\\x20")])
        files: "qtcreator.rc"
    }

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
}
