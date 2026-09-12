import qbs

QtcProduct {
    Depends { name: "bundle" }
    Depends { name: "codesign" }
    Depends { name: "ib"; condition: qbs.targetOS.contains("macos") }

    Properties {
        condition: qbs.targetOS.contains("macos")
        ib.appIconName: "qtcreator"
        // The profiler's call-stack sampler attaches with task_for_pid(), which
        // the kernel only grants to a binary carrying the debugger entitlement;
        // an unsigned build from source carries none. Same entitlements the
        // CMake build applies through qtc_sign_with_entitlements().
        codesign.enableCodeSigning: true
        codesign.signingType: "ad-hoc"
    }

    Group {
        name: "entitlements"
        condition: qbs.targetOS.contains("macos")
        files: ["../../dist/installer/mac/Qt Creator.entitlements"]
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
    installTags: (isBundle ? ["bundle.content"] : base).concat(["debuginfo_app"])
    property bool qtcRunnable: true

    windowsFileDescription: qtc.ide_display_name
    windowsIconPath: sourceDirectory
    windowsResourceFile: sourceDirectory + "/qtcreator.rc"

    bundle.identifier: qtc.ide_bundle_identifier

    // Some of these are in here only to override the entries added to app-Info.plist with other
    // build systems in mind.
    bundle.infoPlist: ({
        "NSHumanReadableCopyright": qtc.ide_copyright_string,
        "CFBundleExecutable": qtc.ide_app_target,
        "CFBundleIdentifier": qtc.ide_bundle_identifier,
        "CFBundleVersion": version,
        "NSAppleEventsUsageDescription": "An application launched via " + qtc.ide_app_target
                                         + " would like to access AppleScript."
    })

    cpp.rpaths: qbs.targetOS.contains("macos") ? ["@executable_path/../Frameworks"]
                                             : ["$ORIGIN/../" + qtc.libDirName + "/qtcreator"]
    cpp.includePaths: [
        project.sharedSourcesDir + "/qtsingleapplication",
    ]

    cpp.frameworks: base.concat(qbs.targetOS.contains("macos") ? ["Foundation"] : [])

    Depends { name: "app_version_header" }
    Depends { name: "Qt"; submodules: ["widgets", "network"] }
    Depends { name: "Utils" }
    Depends { name: "ExtensionSystem" }

    files: [
        "app-Info.plist",
        "app_logo.qrc",
        "macos/qtcreator.xcassets",
        "main.cpp",
        "../shared/qtsingleapplication/qtsingleapplication.h",
        "../shared/qtsingleapplication/qtsingleapplication.cpp",
        "../shared/qtsingleapplication/qtlocalpeer.h",
        "../shared/qtsingleapplication/qtlocalpeer.cpp",
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
        name: "main_macos"
        condition: qbs.targetOS.contains("macos")
        files: [
            "main_mac.mm"
        ]
    }
}
