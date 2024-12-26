// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuick3D as QtQuick3D

import QuickQanava as Qan

import NodeGraphEditorBackend
import NodeGraphEditorBackend

Base {
    id: root

    readonly property QtQuick3D.Texture reset: QtQuick3D.Texture {
    }
    property alias source: root.value.source
    readonly property QtQuick3D.Texture value: QtQuick3D.Texture {
    }

    Layout.preferredHeight: 150
    Layout.preferredWidth: 150
    type: "Texture"

    portsMetaData: QtObject {
        property var pin: [
            {
                id: "texture_in_source",
                alias: "source",
                name: "Source",
                type: "QUrl"
            },
        ]
        property var pout: []
    }

    Component.onCompleted: {
        node.label = "Texture";
        internal.configurePorts(root.graph);
    }
    onValueChanged: {
        NodeGraphEditorBackend.nodeGraphEditorModel.hasUnsavedChanges = true;
    }

    Image {
        anchors.centerIn: parent
        height: 96
        source: root.value.source
        width: 96
    }

    QtObject {
        id: internal

        function configurePorts(graph) {
            const inPorts = NodeGraphEditorBackend.widget.createMetaData_inPorts("QtQuick3D.Texture");
            inPorts.forEach(data => {
                const portItem = graph.insertPort(root.node, Qan.NodeItem.Left, Qan.PortItem.In, `${data.displayName} (${data.type})`, data.id);
                portItem.dataName = data.displayName;
                portItem.dataType = data.type;
            });

            const valuePort = graph.insertPort(root.node, Qan.NodeItem.Right, Qan.PortItem.Out, "OUT", "texture");
            valuePort.dataType = "Texture";
        }
    }
}
