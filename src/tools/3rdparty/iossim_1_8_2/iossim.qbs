import qbs 1.0
import QtcTool


QtcTool {
    name: "iossim"
    condition: qbs.targetOS.contains("osx")

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
        "iphonesimulatorremoteclient/iphonesimulatorremoteclient.h"
    ]
    cpp.linkerFlags: base.concat(["-fobjc-link-runtime"])
    cpp.frameworks: base.concat(["Foundation", "CoreServices", "ApplicationServices", "IOKit",
                                 "AppKit"])
    cpp.frameworkPaths: base.concat("/System/Library/PrivateFrameworks")
    cpp.infoPlistFile: "Info.plist"

    toolInstallDir: project.ide_libexec_path + "/ios"
}
