import qbs.base 1.0
import "../QtcLibrary.qbs" as QtcLibrary

QtcLibrary {
    name: "zeroconf"

    Depends { name: "cpp" }
    Depends { name: "Qt.network" }
    cpp.includePaths: base.concat(".")

    cpp.defines: base.concat("ZEROCONF_LIBRARY")

    Properties {
        condition: qbs.targetOS == "windows"
        cpp.dynamicLibraries:  "ws2_32"
    }
    Properties {
        condition: qbs.targetOS == "linux"
        cpp.defines: base.concat([
            "_GNU_SOURCE",
            "HAVE_IPV6",
            "USES_NETLINK",
            "HAVE_LINUX",
            "TARGET_OS_LINUX"
        ])
    }

    files: [
        "avahiLib.cpp",
        "dnsSdLib.cpp",
        "dns_sd_types.h",
        "embeddedLib.cpp",
        "mdnsderived.cpp",
        "mdnsderived.h",
        "servicebrowser.cpp",
        "servicebrowser.h",
        "servicebrowser_p.h",
        "syssocket.h",
        "zeroconf_global.h",
    ]
}
