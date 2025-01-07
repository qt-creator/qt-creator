// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

import QuickQanava as Qan
import NodeGraphEditorBackend

Qan.Graph {
    id: root

    connectorCreateDefaultEdge: false
    connectorEnabled: true

    portDelegate: Port {
    }

    onConnectorRequestEdgeCreation: (srcNode, dstNode, srcPortItem, dstPortItem) => {
        if (dstPortItem.dataType !== "Any") {
            if (srcPortItem.dataType !== dstPortItem.dataType) {
                return;
            }
        }

        const edge = root.insertEdge(srcNode, dstNode);
        root.bindEdge(edge, srcPortItem, dstPortItem);

        if (dstPortItem.dataBinding) {
            dstPortItem.dataBinding(srcNode.item.value);
        } else {
            // TODO: change old implementation
            dstNode.item.value[dstPortItem.dataName] = Qt.binding(() => {
                if (srcPortItem.dataName !== "")
                    return srcNode.item.value[srcPortItem.dataName];

                return srcNode.item.value;
            });
        }
    }
    onEdgeInserted: {
        NodeGraphEditorBackend.nodeGraphEditorModel.hasUnsavedChanges = true;
    }
    onNodeInserted: {
        NodeGraphEditorBackend.nodeGraphEditorModel.hasUnsavedChanges = true;
    }
    onNodeRemoved: node => {
        NodeGraphEditorBackend.nodeGraphEditorModel.hasUnsavedChanges = true;
    }
    onOnEdgeRemoved: edge => {
        const srcNode = edge.getSource();
        const dstNode = edge.getDestination();
        const srcPortItem = edge.item.sourceItem;
        const dstPortItem = edge.item.destinationItem;

        // TODO: add reset binding function
        if (dstPortItem.dataReset) {
            dstPortItem.dataReset();
        } else {
            dstNode.item.value[dstPortItem.dataName] = dstNode.item.reset[dstPortItem.dataName];
        }
        NodeGraphEditorBackend.nodeGraphEditorModel.hasUnsavedChanges = true;
    }
}
