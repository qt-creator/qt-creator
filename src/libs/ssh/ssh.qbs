import qbs.base 1.0
import "../QtcLibrary.qbs" as QtcLibrary

QtcLibrary {
    name: "QtcSsh"

    cpp.defines: base.concat(["QSSH_LIBRARY"])
    cpp.includePaths: [
        ".",
        "..",
        "../..",
        buildDirectory
    ]

    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: ["widgets", "network" ] }
    Depends { name: "Botan" }

    files: [
        "sftpchannel.h", "sftpchannel_p.h", "sftpchannel.cpp",
        "sftpdefs.cpp", "sftpdefs.h",
        "sftpincomingpacket.cpp", "sftpincomingpacket_p.h",
        "sftpoperation.cpp", "sftpoperation_p.h",
        "sftpoutgoingpacket.cpp", "sftpoutgoingpacket_p.h",
        "sftppacket.cpp", "sftppacket_p.h",
        "sshcapabilities_p.h", "sshcapabilities.cpp",
        "sshchannel.cpp", "sshchannel_p.h",
        "sshchannelmanager.cpp", "sshchannelmanager_p.h",
        "sshconnection.h", "sshconnection_p.h", "sshconnection.cpp",
        "sshconnectionmanager.cpp", "sshconnectionmanager.h",
        "sshcryptofacility.cpp", "sshcryptofacility_p.h",
        "sshkeyexchange.cpp", "sshkeyexchange_p.h",
        "sshkeypasswordretriever_p.h",
        "sshoutgoingpacket.cpp", "sshoutgoingpacket_p.h",
        "sshpacket.cpp", "sshpacket_p.h",
        "sshpacketparser.cpp", "sshpacketparser_p.h",
        "sshremoteprocess.cpp", "sshremoteprocess.h", "sshremoteprocess_p.h",
        "sshdirecttcpiptunnel.h", "sshdirecttcpiptunnel_p.h", "sshdirecttcpiptunnel.cpp",
        "sshremoteprocessrunner.cpp", "sshremoteprocessrunner.h",
        "sshsendfacility.cpp", "sshsendfacility_p.h",
        "sshkeypasswordretriever.cpp",
        "sshkeygenerator.cpp", "sshkeygenerator.h",
        "sshkeycreationdialog.cpp", "sshkeycreationdialog.h", "sshkeycreationdialog.ui",
        "sftpfilesystemmodel.cpp", "sftpfilesystemmodel.h",
        "sshincomingpacket_p.h", "sshincomingpacket.cpp",
        "ssherrors.h",
        "sshexception_p.h",
        "sshpseudoterminal.h",
        "sshbotanconversions_p.h"
    ]

    ProductModule {
        Depends { name: "cpp" }
        Depends { name: "Qt"; submodules: ["widgets", "network"] }
        cpp.includePaths: [".."]
    }
}
