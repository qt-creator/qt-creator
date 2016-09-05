import qbs 1.0

QtcTool {
    name: "iossim"
    condition: qbs.targetOS.contains("macos")

    Depends { name: "bundle" }
    Depends { name: "Qt"; submodules: ["widgets"] }
    Depends { name: "app_version_header" }

    files: [
        "main.mm",
        "nsprintf.mm",
        "nsstringexpandpath.mm",
        "iphonesimulator.mm",
        "iphonesimulator.h",
        "nsprintf.h",
        "nsstringexpandpath.h",
        "version.h",
        "dvtiphonesimulatorremoteclient/dvtiphonesimulatorremoteclient.h"
    ]
    cpp.includePaths: ["."]
    cpp.driverFlags: base.concat(["-fobjc-link-runtime"])
    cpp.frameworks: base.concat(["Foundation", "CoreServices", "ApplicationServices", "IOKit",
                                 "AppKit"])
    cpp.frameworkPaths: base.concat("/System/Library/PrivateFrameworks")
    bundle.infoPlistFile: "Info.plist"

    installDir: qtc.ide_libexec_path + "/ios"
}
