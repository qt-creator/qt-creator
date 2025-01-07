// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts

import QuickQanava as Qan
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls

import NodeGraphEditorBackend

ValueNode {
    id: root

    property alias sourceComponent: loader.sourceComponent
    property alias text: label.text

    height: 100
    width: 150

    metadata: QtObject {
        property var ports: [
            {
                dock: Qan.NodeItem.Right,
                type: Qan.PortItem.Out,
                text: "",
                id: "value_output"
            }
        ]
    }

    Rectangle {
        anchors.fill: parent
        color: "white"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 3
            spacing: 0

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.margins: 2

                Text {
                    id: label

                    anchors.centerIn: parent
                    text: "{{placeholder}}"
                }
            }

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.margins: 2

                Loader {
                    id: loader

                    anchors.centerIn: parent
                }
            }
        }
    }
}
