// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

import QuickQanava as Qan
import Editor as Editor
import Nodes as Nodes

import NodeGraphEditorBackend

Item {
    id: root

    property string nodeGraphFileName

    // Invoked after save changes is done
    property var onSaveChangesCallback: () => {}

    function initEdges(edges) {
        for (let edge in edges) {
            let outNodeUuid = edges[edge].outNodeUuid;
            let inNodeUuid = edges[edge].inNodeUuid;
            let n1, n2;

            for (let i = 0; i < graphView.graph.getNodeCount(); i++) {
                if (graphView.graph.nodes.at(i).item.uuid == outNodeUuid) {
                    n1 = graphView.graph.nodes.at(i);
                    break;
                }
            }

            for (let i = 0; i < graphView.graph.getNodeCount(); i++) {
                if (graphView.graph.nodes.at(i).item.uuid == inNodeUuid) {
                    n2 = graphView.graph.nodes.at(i);
                    break;
                }
            }
            let e = graphView.graph.insertEdge(n1, n2);
            let p1 = n1.item.findPort(edges[edge].outPortName);
            let p2 = n2.item.findPort(edges[edge].inPortName);

            graphView.graph.bindEdge(e, p1, p2);
            if (p2.dataBinding) {
                p2.dataBinding(n1.item.value);
            } else {
                // TODO: change old implementation
                n2.item.value[p2.dataName] = Qt.binding(() => {
                    if (n1.dataName !== "")
                        return n1.item.value[p1.dataName];
                    return n1.item.value;
                });
            }
        }
    }

    function initNodes(nodes) {
        graphView.graph.clearGraph();
        let n = undefined;
        for (let node in nodes) {
            switch (nodes[node].type) {
            case "Material":
                n = graphView.graph.insertNode(Nodes.Components.material);
                break;
            case "Color":
                n = graphView.graph.insertNode(Nodes.Components.color);
                n.item.value.color = nodes[node].value.color;
                break;
            case "Metalness":
                n = graphView.graph.insertNode(Nodes.Components.metalness);
                break;
            case "BaseColor":
                n = graphView.graph.insertNode(Nodes.Components.baseColor);
                break;
            case "RealSpinBox":
                n = graphView.graph.insertNode(Nodes.Components.realSpinBox);
                n.item.value.floating = nodes[node].value.floating;
                break;
            case "Texture":
                n = graphView.graph.insertNode(Nodes.Components.texture);
                break;
            case "CheckBox":
                n = graphView.graph.insertNode(Nodes.Components.checkBox);
                n.item.value.binary = nodes[node].value.binary;
                break;
            case "ComboBox":
                n = graphView.graph.insertNode(Nodes.Components.comboBox);
                n.item.value.text = nodes[node].value.text;
                break;
            case "Roughness":
                n = graphView.graph.insertNode(Nodes.Components.roughness);
                break;
            }
            n.item.x = nodes[node].x;
            n.item.y = nodes[node].y;
            n.item.uuid = nodes[node].uuid;
        }
    }

    // Invoked from C++ side when open node graph is requested and there are unsaved changes
    function promptToSaveBeforeOpen(newNodeGraphName) {
        nodeGraphFileName = newNodeGraphName;
        saveChangesDialog.open();
    }

    function updateGraphData() {
        let arr = {
            nodes: [],
            edges: []
        };

        for (let i = 0; i < graphView.graph.nodes.length; i++) {
            let node = {
                uuid: graphView.graph.nodes.at(i).item.uuid,
                x: graphView.graph.nodes.at(i).item.x,
                y: graphView.graph.nodes.at(i).item.y,
                type: graphView.graph.nodes.at(i).item.type
            };

            //Set the actual value only to the OUT nodes
            if (node.type == "Color" || node.type == "RealSpinBox" || node.type == "CheckBox" || node.type == "ComboBox") {
                node.value = graphView.graph.nodes.at(i).item.value;
            }
            arr["nodes"].push(node);
        }

        for (let i = 0; i < graphView.graph.edges.length; i++) {
            let edge = {
                outNodeUuid: graphView.graph.edges.at(i).item.sourceItem.node.item.uuid,
                inNodeUuid: graphView.graph.edges.at(i).item.destinationItem.node.item.uuid,
                inPortName: graphView.graph.edges.at(i).item.destinationItem.dataId,
                outPortName: graphView.graph.edges.at(i).item.sourceItem.dataId
            };
            arr["edges"].push(edge);
        }
        let newGraphData = JSON.stringify(arr);
        if (NodeGraphEditorBackend.nodeGraphEditorModel.graphData !== newGraphData) {
            NodeGraphEditorBackend.nodeGraphEditorModel.graphData = newGraphData;
        }
    }

    Connections {
        target: NodeGraphEditorBackend.nodeGraphEditorModel

        onNodeGraphLoaded: {
            graphView.graph.clearGraph();
            let jsonString = NodeGraphEditorBackend.nodeGraphEditorModel.graphData;
            let jsonObject = JSON.parse(jsonString);
            let nodes = jsonObject.nodes;
            let edges = jsonObject.edges;
            initNodes(nodes);
            initEdges(edges);
            NodeGraphEditorBackend.nodeGraphEditorModel.hasUnsavedChanges = false;
        }
    }

    SaveAsDialog {
        id: saveAsDialog

        anchors.centerIn: parent

        onSave: {
            graphView.graph.clearGraph();
            graphView.graph.insertNode(Nodes.Components.material);
            NodeGraphEditorBackend.nodeGraphEditorModel.createQmlComponent(graphView.graph);
            updateGraphData();
            NodeGraphEditorBackend.nodeGraphEditorModel.saveFile(fileName);
            NodeGraphEditorBackend.nodeGraphEditorModel.openFileName(fileName);
        }
    }

    SaveChangesDialog {
        id: saveChangesDialog

        anchors.centerIn: parent

        onDiscard: {
            NodeGraphEditorBackend.nodeGraphEditorModel.currentFileName = nodeGraphFileName;
            NodeGraphEditorBackend.nodeGraphEditorModel.openFileName(NodeGraphEditorBackend.nodeGraphEditorModel.currentFileName);
        }
        onRejected: {}
        onSave: {
            if (NodeGraphEditorBackend.nodeGraphEditorModel.currentFileName !== "") {
                NodeGraphEditorBackend.nodeGraphEditorModel.createQmlComponent(graphView.graph);

                /*Save current graph data to the backend*/
                updateGraphData();

                /*Save old data, before opening new node graph*/
                NodeGraphEditorBackend.nodeGraphEditorModel.saveFile(NodeGraphEditorBackend.nodeGraphEditorModel.currentFileName);
                NodeGraphEditorBackend.nodeGraphEditorModel.currentFileName = root.nodeGraphFileName;

                /*Open new node graph*/
                NodeGraphEditorBackend.nodeGraphEditorModel.openFileName(NodeGraphEditorBackend.nodeGraphEditorModel.currentFileName);
                NodeGraphEditorBackend.nodeGraphEditorModel.hasUnsavedChanges = false;
            } else {
                saveAsDialog.open();
            }
        }
    }

    Editor.ContextMenu {
        id: contextMenu

        graph: graphView.graph
    }

    Column {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: toolbar

            color: StudioTheme.Values.themeToolbarBackground
            height: StudioTheme.Values.toolbarHeight
            width: parent.width

            Row {
                anchors.bottomMargin: StudioTheme.Values.toolbarVerticalMargin
                anchors.fill: parent
                anchors.leftMargin: StudioTheme.Values.toolbarHorizontalMargin
                anchors.rightMargin: StudioTheme.Values.toolbarHorizontalMargin
                anchors.topMargin: StudioTheme.Values.toolbarVerticalMargin
                spacing: 6

                HelperWidgets.AbstractButton {
                    buttonIcon: StudioTheme.Constants.add_medium
                    style: StudioTheme.Values.viewBarButtonStyle
                    tooltip: qsTr("Add a new graph node.")

                    onClicked: () => {
                        saveAsDialog.open();
                    }
                }

                HelperWidgets.AbstractButton {
                    buttonIcon: StudioTheme.Constants.save_medium
                    enabled: NodeGraphEditorBackend.nodeGraphEditorModel.hasUnsavedChanges && NodeGraphEditorBackend.nodeGraphEditorModel.currentFileName != ""
                    style: StudioTheme.Values.viewBarButtonStyle
                    tooltip: qsTr("Save.")

                    onClicked: () => {
                        if (NodeGraphEditorBackend.nodeGraphEditorModel.currentFileName !== "") {
                            NodeGraphEditorBackend.nodeGraphEditorModel.createQmlComponent(graphView.graph);
                            updateGraphData();
                            NodeGraphEditorBackend.nodeGraphEditorModel.hasUnsavedChanges = false;
                            NodeGraphEditorBackend.nodeGraphEditorModel.saveFile(NodeGraphEditorBackend.nodeGraphEditorModel.currentFileName);
                        } else {
                            saveAsDialog.open();
                        }
                    }
                }

                Label {
                    text: NodeGraphEditorBackend.nodeGraphEditorModel.currentFileName
                }
            }
        }

        Rectangle {
            id: graphContent

            clip: true
            color: StudioTheme.Values.themePanelBackground
            height: parent.height - toolbar.height
            width: parent.width

            Qan.GraphView {
                id: graphView

                anchors.fill: parent
                navigable: true

                graph: Editor.Graph {
                }

                Keys.onPressed: event => {
                    if (event.key === Qt.Key_Delete) {
                        graph.removeSelection();
                    }
                }
                onRightClicked: function (pos) {
                    if (NodeGraphEditorBackend.nodeGraphEditorModel.currentFileName !== "") {
                        contextMenu.popup();
                    }
                }
            }

            Editor.NewNodeGraphDialog {
                id: newNodeGraphDialog

            }
        }
    }
}
