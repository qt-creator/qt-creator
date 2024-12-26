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
        property real occlusion
    }
    readonly property QtObject value: QtObject {
        property int channel
        property QtQuick3D.Texture map
        property real occlusion
    }

    Layout.preferredHeight: 150
    Layout.preferredWidth: 150
    type: "Occlusion"

    portsMetaData: QtObject {
        property var pin: [
            {
                id: "occlusion",
                alias: "occlusion",
                name: "Occlusion",
                type: "real"
            },
            {
                id: "occlusion_in_occlusionChannel",
                alias: "channel",
                name: "Channel",
                type: "QQuick3DMaterial::TextureChannelMapping"
            },
            {
                id: "occlusion_in_occlusionMap ",
                alias: "map",
                name: "Map",
                type: "Texture"
            },
        ]
        property var pout: [
            {
                id: "occlusion_out",
                alias: "",
                name: "OUT",
                type: "nge::Occlusion"
            },
        ]
    }

    Component.onCompleted: {
        node.label = "Occlusion";
    }
}
