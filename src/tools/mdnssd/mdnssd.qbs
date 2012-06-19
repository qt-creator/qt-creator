import qbs.base 1.0
import qbs.fileinfo 1.0 as FileInfo
import "../QtcTool.qbs" as QtcTool

QtcTool {
    name: "mdnssd"

    cpp.defines: [
        'PID_FILE="/tmp/mdnsd.pid"',
        'MDNS_UDS_SERVERPATH="/tmp/mdnsd"',
        "MDNS_DEBUGMSGS=0",
        "HAVE_IPV6"
    ]
    cpp.includePaths: [
        ".",
        buildDirectory
    ]

    Depends { name: "cpp" }

    files: [
        "uds_daemon.c",
        "uds_daemon.h",
        "uDNS.c",
        "uDNS.h",
        "mDNSDebug.c",
        "mDNSDebug.h",
        "GenLinkedList.c",
        "GenLinkedList.h",
        "dnssd_ipc.c",
        "dnssd_ipc.h",
        "DNSDigest.c",
        "DNSCommon.c",
        "mDNSUNP.h",
        "mDNSEmbeddedAPI.h",
        "DNSCommon.h",
        "DebugServices.h",
        "dns_sd.h",
        "mDNS.c"
    ]

    Group {
        condition: qbs.targetOS == "linux"
        files: [
            "mDNSPosix.c",
            "mDNSPosix.h",
            "PlatformCommon.c",
            "PlatformCommon.h",
            "PosixDaemon.c",
            "mDNSUNP.c"
        ]
    }

    Properties {
        condition: qbs.targetOS == "linux"
        cpp.defines: outer.concat([
            "_GNU_SOURCE",
            "NOT_HAVE_SA_LEN",
            "USES_NETLINK",
            "HAVE_LINUX",
            "TARGET_OS_LINUX"
        ])
    }

    Properties {
        condition: qbs.targetOS == "macx"
        cpp.defines: outer.concat([
            "__MAC_OS_X_VERSION_MIN_REQUIRED=__MAC_OS_X_VERSION_10_4",
            "__APPLE_USE_RFC_2292"
        ])
    }

    Group {
        condition: qbs.targetOS == "windows"
        files: [
            "Firewall.h",
            "Firewall.cpp",
            "mDNSWin32.h",
            "mDNSWin32.c",
            "Poll.h",
            "Poll.c",
            "Secret.h",
            "Secret.c",
            "Service.h",
            "Service.c",
            "DebugServices.c",
            "LegacyNATTraversal.c",
            "resource.h",
            "RegNames.h",
            "CommonServices.h",
            "main.c",
            "EventLog.mc",
            "Service.rc"
        ]
    }

    Properties {
        condition: qbs.targetOS == "windows"
        cpp.includePaths: outer.concat([buildDirectory + "/" + name])
        cpp.defines: outer.concat([
            "WIN32",
            "_WIN32_WINNT=0x0501",
            "NDEBUG",
            "MDNS_DEBUGMSGS=0",
            "TARGET_OS_WIN32",
            "WIN32_LEAN_AND_MEAN",
            "USE_TCP_LOOPBACK",
            "PLATFORM_NO_STRSEP",
            "PLATFORM_NO_EPIPE",
            "PLATFORM_NO_RLIMIT",
            "UNICODE",
            "_UNICODE",
            "_CRT_SECURE_NO_DEPRECATE",
            "_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1",
            "_LEGACY_NAT_TRAVERSAL_"
        ])
        cpp.dynamicLibraries: [
            "ws2_32.lib",
            "advapi32.lib",
            "ole32.lib",
            "oleaut32.lib",
            "iphlpapi.lib",
            "netapi32.lib",
            "user32.lib",
            "powrprof.lib",
            "shell32.lib"
        ]
    }

    Properties {
        condition: qbs.toolchain == 'mingw' || qbs.toolchain == 'gcc'
        cpp.cFlags: [
            "-Wno-unused-but-set-variable",
            "-Wno-strict-aliasing"
        ]
        cpp.cxxFlags: [
            "-Wno-unused-but-set-variable"
        ]
    }

    FileTagger {
        pattern: "*.mc"
        fileTags: "mc"
    }

    Rule {
        inputs: "mc"
        Artifact {
            fileName: product.name + "/" + input.baseName + ".h"
            fileTags: "hpp"
        }
        Artifact {
            fileName: product.name + "/" + input.baseName + ".rc"
            fileTags: "mc_result"
        }
        prepare: {
            var cmd = new Command("mc", input.fileName);
            cmd.workingDirectory = FileInfo.path(outputs["mc_result"][0].fileName);
            cmd.description = "mc " + FileInfo.fileName(input.fileName);
            return cmd;
        }
    }
}

