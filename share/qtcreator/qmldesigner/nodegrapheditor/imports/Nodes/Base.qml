// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts

import QuickQanava as Qan

import NodeGraphEditorBackend

Qan.NodeItem {
    id: root

    default property alias data: content.data
    property QtObject portsMetaData: QtObject {
        // {
        //     id: "",
        //     alias: "",
        //     name: "",
        //     type: "",
        //     binding: (values) => {}
        // }
        property var pin: []
        property var pout: []
    }
    readonly property string uuid: NodeGraphEditorBackend.widget.generateUUID()

    Layout.preferredHeight: 60
    Layout.preferredWidth: 100
    connectable: Qan.NodeItem.UnConnectable
    height: Layout.preferredHeight
    width: Layout.preferredWidth

    Component.onCompleted: {
        internal.configurePorts(root.graph);
    }

    Qan.RectNodeTemplate {
        anchors.fill: parent
        nodeItem: parent

        Item {
            id: content

            anchors.fill: parent
        }
    }

    QtObject {
        id: internal

        function configurePorts(graph) {
            const initPort = (portItem, data) => {
                if (data.binding) {
                    portItem.dataBinding = data.binding;
                }
                portItem.dataName = data.alias;
                portItem.dataType = data.type;
            };

            root.portsMetaData.pin.forEach(data => {
                const portItem = graph.insertPort(root.node, Qan.NodeItem.Left, Qan.PortItem.In, `${data.name} (${data.type})`, data.id);
                initPort(portItem, data);
            });

            root.portsMetaData.pout.forEach(data => {
                const portItem = graph.insertPort(root.node, Qan.NodeItem.Right, Qan.PortItem.Out, `${data.name}`, data.id);
                initPort(portItem, data);
            });
        }
    }
}
