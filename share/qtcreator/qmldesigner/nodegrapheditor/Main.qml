// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

import QuickQanava as Qan
import Editor as Editor

import NodeGraphEditorBackend

Item {
    id: root

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
                        newNodeGraphDialog.open();
                    }
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
                    contextMenu.popup();
                }
            }

            Editor.NewNodeGraphDialog {
                id: newNodeGraphDialog

            }
        }
    }
}
