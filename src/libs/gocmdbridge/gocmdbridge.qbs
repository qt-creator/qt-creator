Project {
    name: "gocmdbridge"
    property string magicPacketMarker: "PkgMarkerGoBridgeMagicPacket"
    // Build the C implementation of the bridge server instead of the Go one.
    // Equivalent to -DBUILD_CMDBRIDGE_C=ON in the CMake build.
    property bool buildCmdBridgeC: false

    references: [
        "client/client.qbs",
        "server/server.qbs",
    ]
}
