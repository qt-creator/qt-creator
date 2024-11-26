// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Rectangle {
    id: root

    property var rootView: shaderEditor

    color: StudioTheme.Values.themeToolbarBackground

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            Layout.topMargin: StudioTheme.Values.toolbarVerticalMargin
            Layout.leftMargin: StudioTheme.Values.toolbarHorizontalMargin
            Layout.rightMargin: StudioTheme.Values.toolbarHorizontalMargin

            StudioControls.TopLevelComboBox {
                id: nodesComboBox
                style: StudioTheme.Values.toolbarStyle
                Layout.preferredWidth: 400
                Layout.alignment: Qt.AlignVCenter
                model: editableCompositionsModel
                textRole: "display"
                Binding on currentIndex {
                    value: editableCompositionsModel.selectedIndex
                }
                onActivated: (idx) => {
                    editableCompositionsModel.openCodeEditor(idx)
                }
            }

            Item { // Spacer
                Layout.preferredHeight: 1
                Layout.fillWidth: true
            }

            ColumnChooser {
                table: uniformsView.tableView
                text: qsTr("Columns")
                style: StudioTheme.Values.viewBarControlStyle
                Layout.alignment: Qt.AlignVCenter
            }

            StudioControls.CheckBox {
                text: qsTr("Live Update")
                actionIndicatorVisible: false
                style: StudioTheme.Values.viewBarControlStyle
                checked: root.rootView ? root.rootView.liveUpdate : false
                onToggled: root.rootView.liveUpdate = checked
                Layout.alignment: Qt.AlignVCenter
            }
        }

        CodeEditorUniformsView {
            id: uniformsView

            Layout.fillWidth: true
            Layout.fillHeight: true
            model: uniformsTableModel
        }
    }
}
