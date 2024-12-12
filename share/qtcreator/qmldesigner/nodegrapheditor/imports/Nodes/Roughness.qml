// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts

import QtQuick3D as QtQuick3D

Base {
    id: root

    readonly property QtObject reset: QtObject {
        property int channel
        property QtQuick3D.Texture map
        property real roughness
    }
    readonly property QtObject value: QtObject {
        property int channel
        property QtQuick3D.Texture map
        property real roughness
    }

    Layout.preferredHeight: 150
    Layout.preferredWidth: 150

    portsMetaData: QtObject {
        property var pin: [
            {
                id: "roughness_in_roughness",
                alias: "roughness",
                name: "Roughness",
                type: "real"
            },
            {
                id: "roughness_in_roughnessChannel",
                alias: "channel",
                name: "Channel",
                type: "QQuick3DMaterial::TextureChannelMapping"
            },
            {
                id: "roughness_in_roughnessMap ",
                alias: "map",
                name: "Map",
                type: "Texture"
            },
        ]
        property var pout: [
            {
                id: "roughness_out",
                alias: "",
                name: "OUT",
                type: "nge::Roughness"
            },
        ]
    }

    Component.onCompleted: {
        node.label = "Roughness";
    }
}
