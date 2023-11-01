import qbs 1.0

QtcTool {
    name: "iostool"
    condition: qbs.targetOS.contains("macos")

    Depends { name: "bundle" }
    Depends { name: "Qt.widgets" }
    Depends { name: "Qt.xml" }
    Depends { name: "Qt.network" }

    files: [
        "cfutils.h",
        "Info.plist",
        "gdbrunner.cpp",
        "gdbrunner.h",
        "iosdevicemanager.cpp",
        "iosdevicemanager.h",
        "iostool.cpp",
        "iostool.h",
        "iostooltypes.h",
        "main.cpp",
        "mobiledevicelib.cpp",
        "mobiledevicelib.h",
        "relayserver.cpp",
        "relayserver.h"
    ]
    cpp.frameworks: base.concat(["CoreFoundation", "CoreServices", "IOKit", "Security",
                                 "SystemConfiguration"])

    installDir: qtc.ide_libexec_path + "/ios"
}
