// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts

import QtQuick3D as QtQuick3D

Base {
    id: root

    readonly property QtObject reset: QtObject {
        property int channel
        property color color
        property QtQuick3D.Texture map
        property bool singleChannelEnabled
    }
    readonly property QtObject value: QtObject {
        property int channel
        property color color
        property QtQuick3D.Texture map
        property bool singleChannelEnabled
    }

    Layout.preferredHeight: 150
    Layout.preferredWidth: 150
    type: "BaseColor"

    portsMetaData: QtObject {
        property var pin: [
            {
                id: "basecolor_in_baseColor",
                alias: "color",
                name: "Color",
                type: "QColor"
            },
            {
                id: "basecolor_in_baseColorMap",
                alias: "map",
                name: "Map",
                type: "Texture"
            },
            {
                id: "basecolor_in_baseColorSingleChannelEnabled",
                alias: "singleChannelEnabled",
                name: "Single Channel Enabled",
                type: "bool"
            },
            {
                id: "basecolor_in_baseColorChannel",
                alias: "channel",
                name: "Channel",
                type: "QQuick3DMaterial::TextureChannelMapping"
            },
        ]
        property var pout: [
            {
                id: "basecolor_out",
                alias: "",
                name: "OUT",
                type: "nge::BaseColor"
            },
        ]
    }

    Component.onCompleted: {
        node.label = "Base Color";
    }
}
