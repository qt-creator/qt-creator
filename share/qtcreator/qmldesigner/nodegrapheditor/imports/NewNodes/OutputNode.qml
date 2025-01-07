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

    property alias text: label.text

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

    RowLayout {
        anchors.fill: parent
        anchors.margins: 3
        spacing: 0

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.margins: 2

            Text {
                id: label

                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: "{{placeholder}}"
            }
        }
    }
}
