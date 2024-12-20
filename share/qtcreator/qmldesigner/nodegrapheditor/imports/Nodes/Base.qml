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
        //     enabled: true,
        //     binding: (values) => {}
        // }
        property var pin: []
        property var pout: []
    }
    property string type
    property string uuid: NodeGraphEditorBackend.widget.generateUUID()

    function switchPin(pin_id) {
        root.portsMetaData.pin.forEach(data => {
            if (data.id == pin_id) {
                if (data.enabled === false)
                    data.enabled = true;
                else
                    data.enabled = false;
            }
        });
    }

    function updatePinVisibility(pin_id) {
        internal.createdPorts.forEach(data => {
            if (data.dataId === pin_id) {
                root.portsMetaData.pin.forEach(data2 => {
                    if (data2.id === pin_id) {
                        data.visible = data2.enabled;
                    }
                });
            }
        });
    }

    Layout.preferredHeight: 60
    Layout.preferredWidth: 100
    connectable: Qan.NodeItem.UnConnectable
    height: Layout.preferredHeight
    width: Layout.preferredWidth

    Component.onCompleted: {
        internal.configurePorts(root.graph);
    }
    onXChanged: {
        NodeGraphEditorBackend.nodeGraphEditorModel.hasUnsavedChanges = true;
    }
    onYChanged: {
        NodeGraphEditorBackend.nodeGraphEditorModel.hasUnsavedChanges = true;
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

        property var createdPorts: []

        function configurePorts(graph) {
            const initPort = (portItem, data) => {
                if (data.binding) {
                    portItem.dataBinding = data.binding;
                }
                portItem.dataName = data.alias;
                portItem.dataType = data.type;
                portItem.dataId = data.id;
            };
            for (var i = 0; i < createdPorts.length; i++) {
                graph.removePort(root.node, createdPorts[i]);
            }
            createdPorts = [];

            root.node.item.ports.clear();

            root.portsMetaData.pin.forEach(data => {
                const portItem = graph.insertPort(root.node, Qan.NodeItem.Left, Qan.PortItem.In, `${data.name} (${data.type})`, data.id);
                initPort(portItem, data);

                if (data.enabled != undefined && data.enabled === false) {
                    portItem.visible = false;
                }
                createdPorts.push(portItem);
            });

            root.portsMetaData.pout.forEach(data => {
                const portItem = graph.insertPort(root.node, Qan.NodeItem.Right, Qan.PortItem.Out, `${data.name}`, data.id);
                initPort(portItem, data);
                createdPorts.push(portItem);
            });
        }
    }
}
