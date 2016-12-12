import qbs 1.0
import qbs.Environment

Project {
    name: "QtcSsh"

    QtcDevHeaders { }

    QtcLibrary {
        cpp.defines: base.concat(["QTCSSH_LIBRARY"]).concat(botanDefines)
        cpp.includePaths: botanIncludes
        cpp.dynamicLibraries: botanLibs
        cpp.enableExceptions: true

        Depends { name: "Qt"; submodules: ["widgets", "network" ] }

        files: [
            "sftpchannel.h", "sftpchannel_p.h", "sftpchannel.cpp",
            "sftpdefs.cpp", "sftpdefs.h",
            "sftpfilesystemmodel.cpp", "sftpfilesystemmodel.h",
            "sftpincomingpacket.cpp", "sftpincomingpacket_p.h",
            "sftpoperation.cpp", "sftpoperation_p.h",
            "sftpoutgoingpacket.cpp", "sftpoutgoingpacket_p.h",
            "sftppacket.cpp", "sftppacket_p.h",
            "sshbotanconversions_p.h",
            "sshcapabilities_p.h", "sshcapabilities.cpp",
            "sshchannel.cpp", "sshchannel_p.h",
            "sshchannelmanager.cpp", "sshchannelmanager_p.h",
            "sshconnection.h", "sshconnection_p.h", "sshconnection.cpp",
            "sshconnectionmanager.cpp", "sshconnectionmanager.h",
            "sshcryptofacility.cpp", "sshcryptofacility_p.h",
            "sshdirecttcpiptunnel.h", "sshdirecttcpiptunnel_p.h", "sshdirecttcpiptunnel.cpp",
            "ssherrors.h",
            "sshexception_p.h",
            "sshforwardedtcpiptunnel.cpp", "sshforwardedtcpiptunnel.h", "sshforwardedtcpiptunnel_p.h",
            "sshhostkeydatabase.cpp",
            "sshhostkeydatabase.h",
            "sshincomingpacket_p.h", "sshincomingpacket.cpp",
            "sshinit_p.h", "sshinit.cpp",
            "sshkeycreationdialog.cpp", "sshkeycreationdialog.h", "sshkeycreationdialog.ui",
            "sshkeyexchange.cpp", "sshkeyexchange_p.h",
            "sshkeygenerator.cpp", "sshkeygenerator.h",
            "sshkeypasswordretriever.cpp",
            "sshkeypasswordretriever_p.h",
            "sshlogging.cpp", "sshlogging_p.h",
            "sshoutgoingpacket.cpp", "sshoutgoingpacket_p.h",
            "sshpacket.cpp", "sshpacket_p.h",
            "sshpacketparser.cpp", "sshpacketparser_p.h",
            "sshpseudoterminal.h",
            "sshremoteprocess.cpp", "sshremoteprocess.h", "sshremoteprocess_p.h",
            "sshremoteprocessrunner.cpp", "sshremoteprocessrunner.h",
            "sshsendfacility.cpp", "sshsendfacility_p.h",
            "sshtcpipforwardserver.cpp", "sshtcpipforwardserver.h", "sshtcpipforwardserver_p.h",
            "sshtcpiptunnel.cpp", "sshtcpiptunnel_p.h",
        ].concat(botanFiles)

        property var useSystemBotan: Environment.getEnv("USE_SYSTEM_BOTAN") === "1"
        property var botanIncludes: {
            var result = ["../3rdparty"];
            if (useSystemBotan)
                result.push("/usr/include/botan-1.10")
            return result
        }
        property var botanLibs: {
            var result = [];
            if (useSystemBotan)
                result.push("botan-1.10")
            if (qbs.targetOS.contains("windows"))
                result.push("advapi32", "user32")
            else if (qbs.targetOS.contains("linux"))
                result.push("rt", "dl");
            else if (qbs.targetOS.contains("macos"))
                result.push("dl");
            else if (qbs.targetOS.contains("unix"))
                result.push("rt");
            return result
        }
        property var botanDefines: {
            var result = [];
            if (useSystemBotan) {
                result.push("USE_SYSTEM_BOTAN")
            } else {
                result.push("BOTAN_DLL=")
                if (qbs.toolchain.contains("msvc"))
                    result.push("BOTAN_BUILD_COMPILER_IS_MSVC",
                                "BOTAN_TARGET_OS_HAS_GMTIME_S",
                                "_SCL_SECURE_NO_WARNINGS")
                if (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("mingw"))
                    result.push("BOTAN_BUILD_COMPILER_IS_GCC")
                if (qbs.targetOS.contains("linux"))
                    result.push("BOTAN_TARGET_OS_IS_LINUX", "BOTAN_TARGET_OS_HAS_CLOCK_GETTIME",
                                "BOTAN_TARGET_OS_HAS_DLOPEN", " BOTAN_TARGET_OS_HAS_GMTIME_R",
                                "BOTAN_TARGET_OS_HAS_POSIX_MLOCK", "BOTAN_HAS_DYNAMICALLY_LOADED_ENGINE",
                                "BOTAN_HAS_DYNAMIC_LOADER", "BOTAN_TARGET_OS_HAS_GETTIMEOFDAY",
                                "BOTAN_HAS_ALLOC_MMAP", "BOTAN_HAS_ENTROPY_SRC_DEV_RANDOM",
                                "BOTAN_HAS_ENTROPY_SRC_EGD", "BOTAN_HAS_ENTROPY_SRC_FTW",
                                "BOTAN_HAS_ENTROPY_SRC_UNIX", "BOTAN_HAS_MUTEX_PTHREAD", "BOTAN_HAS_PIPE_UNIXFD_IO")
                if (qbs.targetOS.contains("macos"))
                    result.push("BOTAN_TARGET_OS_IS_DARWIN", "BOTAN_TARGET_OS_HAS_GETTIMEOFDAY",
                                "BOTAN_HAS_ALLOC_MMAP", "BOTAN_HAS_ENTROPY_SRC_DEV_RANDOM",
                                "BOTAN_HAS_ENTROPY_SRC_EGD", "BOTAN_HAS_ENTROPY_SRC_FTW",
                                "BOTAN_HAS_ENTROPY_SRC_UNIX", "BOTAN_HAS_MUTEX_PTHREAD", "BOTAN_HAS_PIPE_UNIXFD_IO")
                if (qbs.targetOS.contains("windows"))
                    result.push("BOTAN_TARGET_OS_IS_WINDOWS",
                                "BOTAN_TARGET_OS_HAS_LOADLIBRARY", "BOTAN_TARGET_OS_HAS_WIN32_GET_SYSTEMTIME",
                                "BOTAN_TARGET_OS_HAS_WIN32_VIRTUAL_LOCK", "BOTAN_HAS_DYNAMICALLY_LOADED_ENGINE",
                                "BOTAN_HAS_DYNAMIC_LOADER", "BOTAN_HAS_ENTROPY_SRC_CAPI",
                                "BOTAN_HAS_ENTROPY_SRC_WIN32", "BOTAN_HAS_MUTEX_WIN32")
            }
            return result
        }
        property var botanFiles: {
            var result = ["../3rdparty/botan/botan.h"];
            if (!useSystemBotan)
                result.push("../3rdparty/botan/botan.cpp")
            return result
        }

        // For Botan.
        Properties {
            condition: qbs.toolchain.contains("mingw")
            cpp.cxxFlags: base.concat([
                "-fpermissive",
                "-finline-functions",
                "-Wno-long-long"
            ])
        }
        cpp.cxxFlags: base

        Export {
            Depends { name: "Qt"; submodules: ["widgets", "network"] }
        }
    }
}
