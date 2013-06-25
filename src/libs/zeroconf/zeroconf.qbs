import qbs.base 1.0
import "../QtcLibrary.qbs" as QtcLibrary

QtcLibrary {
    name: "zeroconf"

    Depends { name: "Qt.network" }
    cpp.includePaths: base.concat(".")

    cpp.defines: {
        var list = base;
        list.push("ZEROCONF_LIBRARY");
        if (qbs.targetOS.contains("linux")) {
            list.push(
                "_GNU_SOURCE",
                "HAVE_IPV6",
                "USES_NETLINK",
                "HAVE_LINUX",
                "TARGET_OS_LINUX"
            );
        }
        return list;
    }

    Properties {
        condition: qbs.targetOS.contains("windows")
        cpp.dynamicLibraries:  "ws2_32"
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
