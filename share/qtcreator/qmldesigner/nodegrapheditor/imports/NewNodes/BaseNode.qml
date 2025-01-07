// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts

import QuickQanava as Qan
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls

import NodeGraphEditorBackend

Qan.NodeItem {
    id: root

    default property alias data: content.data
    property QtObject metadata: QtObject {
        property var ports: []
    }
    readonly property string uuid: NodeGraphEditorBackend.widget.generateUUID()

    connectable: Qan.NodeItem.UnConnectable
    height: 35
    resizable: false
    width: 350

    Component.onCompleted: {
        internal.initialize();
    }

    QtObject {
        id: internal

        readonly property var initialize: () => {
            const graph = root.graph;

            root.metadata.ports.forEach(data => {
                const port = graph.insertPort(root.node, data.dock, data.type, data.text, data.id);
                port.dataBinding = data.binding;
                port.dataReset = data.reset;

                port.dataType = "Any";
            });
        }
    }

    Item {
        id: content

        anchors.fill: parent
    }
}
