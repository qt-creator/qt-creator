Project {
    name: "gocmdbridge"
    property string magicPacketMarker: "PkgMarkerGoBridgeMagicPacket"

    references: [
        "client/client.qbs",
        "server/server.qbs",
    ]
}
