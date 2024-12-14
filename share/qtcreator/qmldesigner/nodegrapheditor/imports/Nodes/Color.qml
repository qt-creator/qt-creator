// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import NodeGraphEditorBackend

Base {
    id: root

    readonly property QtObject value: QtObject {
        property color color

        onColorChanged: {
            NodeGraphEditorBackend.nodeGraphEditorModel.hasUnsavedChanges = true;
        }
    }

    type: "Color"

    portsMetaData: QtObject {
        property var pin: []
        property var pout: [
            {
                id: "checkbox_out",
                alias: "color",
                name: "OUT",
                type: "QColor"
            },
        ]
    }

    Component.onCompleted: {
        node.label = "Color (QColor)";
    }

    Rectangle {
        anchors.centerIn: parent
        color: root.value.color
        height: 32
        width: 32

        MouseArea {
            anchors.fill: parent

            onClicked: colorPopup.open(root)
        }

        ColorEditorPopup {
            id: colorPopup

            currentColor: root.value.color

            onActivateColor: color => root.value.color = color
        }
    }
}
