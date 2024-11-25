QtcLibrary {
    name: "CmdBridgeClient"
    Depends { name: "Utils" }
    Depends { name: "cpp" }

    cpp.defines: base.concat("CMDBRIDGECLIENT_LIBRARY",
                             "GOBRIDGE_MAGIC_PACKET_MARKER=\"" + project.magicPacketMarker + "\"")

    files: [
        "bridgedfileaccess.cpp",
        "bridgedfileaccess.h",
        "cmdbridgeclient.cpp",
        "cmdbridgeclient.h",
        "cmdbridgeglobal.h",
        "cmdbridgetr.h",
    ]

    Export {
        // bad, but follows the cmake approach
        Depends { name: "cpp" }
        cpp.includePaths: [ exportingProduct.sourceDirectory + "/.." ]
    }
}
