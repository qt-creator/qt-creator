// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts

import QuickQanava as Qan
import StudioControls as StudioControls

import NodeGraphEditorBackend

Qan.GroupItem {
    id: root

    property QtObject metadata: QtObject {
        property var nodes: []
    }
    readonly property string uuid: NodeGraphEditorBackend.widget.generateUUID()

    acceptDrops: false
    container: content
    droppable: false
    height: c.visible ? content.height + header.height : header.height
    resizable: false
    width: content.width

    Component.onCompleted: {
        internal.initialize();
    }
    Component.onDestruction: {
        internal.terminate();
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: header

            Layout.fillWidth: true
            Layout.preferredHeight: 35
            color: "green"

            RowLayout {
                anchors.fill: parent

                StudioControls.AbstractButton {
                    id: buttonAlignTop

                    buttonIcon: "\u0187"

                    onClicked: {
                        c.visible = !c.visible;
                    }
                }

                Text {
                    text: root.node.label
                }
            }
        }

        Rectangle {
            id: c

            Layout.fillHeight: true
            Layout.fillWidth: true
            color: "white"

            Column {
                id: content

                spacing: 3
            }
        }
    }

    QtObject {
        id: internal

        readonly property QtObject data: QtObject {
            readonly property var instances: []
        }
        readonly property var initialize: () => {
            const graph = root.graph;

            root.metadata.nodes.forEach(data => {
                const metadata = Components[data.metadata];
                const node = metadata.constructor(graph, root, data);
                graph.groupNode(root.node, node, null, false);

                internal.data.instances.push(node);
            });
        }
        readonly property var terminate: () => {
            internal.data.instances.forEach(n => {
                graph.removeNode(n);
            });
        }
    }
}
