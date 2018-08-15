import qbs 1.0
import qbs.Environment

Project {
    name: "QtcSsh"

    QtcDevHeaders { }

    QtcLibrary {
        cpp.defines: base.concat("QTCSSH_LIBRARY")
        cpp.includePaths: botanIncludes
        cpp.dynamicLibraries: botanLibs
        cpp.enableExceptions: true

        Properties {
            condition: qbs.toolchain.contains("msvc")
            cpp.cxxFlags: base.concat("/wd4250");
        }

        Depends { name: "Qt"; submodules: ["widgets", "network" ] }
        Depends { name: "Botan"; condition: !qtc.useSystemBotan }

        files: [
            "sftpchannel.h", "sftpchannel_p.h", "sftpchannel.cpp",
            "sftpdefs.cpp", "sftpdefs.h",
            "sftpfilesystemmodel.cpp", "sftpfilesystemmodel.h",
            "sftpincomingpacket.cpp", "sftpincomingpacket_p.h",
            "sftpoperation.cpp", "sftpoperation_p.h",
            "sftpoutgoingpacket.cpp", "sftpoutgoingpacket_p.h",
            "sftppacket.cpp", "sftppacket_p.h",
            "ssh.qrc",
            "sshagent.cpp", "sshagent_p.h",
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
        ]

        property var botanIncludes: qtc.useSystemBotan ? ["/usr/include/botan-2"] : []
        property var botanLibs: {
            var result = [];
            if (qtc.useSystemBotan)
                result.push("botan-2")
            if (qbs.targetOS.contains("windows"))
                result.push("advapi32", "user32", "ws2_32")
            else if (qbs.targetOS.contains("linux"))
                result.push("rt", "dl");
            else if (qbs.targetOS.contains("macos"))
                result.push("dl");
            else if (qbs.targetOS.contains("unix"))
                result.push("rt");
            return result
        }

        Export {
            Depends { name: "Qt"; submodules: ["widgets", "network"] }
        }
    }
}
