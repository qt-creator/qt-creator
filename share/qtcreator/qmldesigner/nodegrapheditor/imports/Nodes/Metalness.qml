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
        property real metalness
    }
    readonly property QtObject value: QtObject {
        property int channel
        property QtQuick3D.Texture map
        property real metalness
    }

    Layout.preferredHeight: 150
    Layout.preferredWidth: 150
    type: "Metalness"

    portsMetaData: QtObject {
        property var pin: [
            {
                id: "metalness_in_metalness",
                alias: "metalness",
                name: "Metalness",
                enabled: true,
                type: "real"
            },
            {
                id: "metalness_in_metalnessChannel",
                alias: "channel",
                name: "Channel",
                enabled: true,
                type: "QQuick3DMaterial::TextureChannelMapping"
            },
            {
                id: "metalness_in_metalnessMap ",
                alias: "map",
                name: "Map",
                enabled: false,
                type: "Texture"
            },
        ]
        property var pout: [
            {
                id: "metalness_out",
                alias: "",
                name: "OUT",
                enabled: true,
                type: "nge::Metalness"
            },
        ]
    }

    Component.onCompleted: {
        node.label = "Metalness";
    }
}
