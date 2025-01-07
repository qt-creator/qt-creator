// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuick3D as QtQuick3D

import QuickQanava as Qan
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls

import NodeGraphEditorBackend

BaseGroup {
    id: root

    readonly property QtObject value: QtObject {
        property int channel
        property QtQuick3D.Texture map
        property real opacity: 1.0
    }

    metadata: QtObject {
        property var nodes: [
            {
                metadata: "input",
                text: "Opacity",
                sourceComponent: "realSpinBox",
                value: "opacity"
            },
            {
                metadata: "input",
                text: "Map",
                value: "map",
                resetValue: null
            },
            {
                metadata: "input",
                text: "Channel",
                sourceComponent: "materialChannel",
                value: "channel"
            },
        ]
    }
}
