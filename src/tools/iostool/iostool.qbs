import qbs 1.0

QtcTool {
    name: "iostool"
    condition: qbs.targetOS.contains("osx")

    Depends { name: "Qt.widgets" }
    Depends { name: "Qt.xml" }
    Depends { name: "Qt.network" }
    Depends { name: "app_version_header" }

    files: [
        "main.cpp",
        "iosdevicemanager.cpp",
        "iosdevicemanager.h"
    ]
    cpp.frameworks: base.concat(["CoreFoundation", "CoreServices", "IOKit", "Security",
                                 "SystemConfiguration"])
    cpp.dynamicLibraries: base.concat(["ssl", "bz2"])
    cpp.infoPlistFile: "Info.plist"

    installDir: project.ide_libexec_path + "/ios"
}
