import qbs

QtcProduct {
    Depends { name: "bundle" }
    Depends { name: "ib"; condition: qbs.targetOS.contains("osx") }

    bundle.isBundle: true
    bundle.infoPlistFile: "Info.plist"

    ib.appIconName: "qtcreator"

    type: ["application"]
    name: project.ide_app_target
    consoleApplication: qbs.debugInformation
    version: project.qtcreator_version

    installSourceBase: buildDirectory

    cpp.rpaths: qbs.targetOS.contains("osx") ? ["@executable_path/../Frameworks"]
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
        fileTagsFilter: ["infoplist", "pkginfo", "compiled_assetcatalog"]
        qbs.install: true
        qbs.installSourceBase: installSourceBase
    }
}
