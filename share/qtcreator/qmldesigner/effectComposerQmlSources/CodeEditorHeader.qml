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
            Item { // Spacer
                Layout.preferredHeight: 1
                Layout.fillWidth: true
            }

            ColumnChooser {
                table: uniformsView.tableView
                text: qsTr("Columns")
                style: StudioTheme.Values.viewBarControlStyle
                Layout.topMargin: StudioTheme.Values.marginTopBottom
            }

            StudioControls.CheckBox {
                text: qsTr("Live Update")
                actionIndicatorVisible: false
                style: StudioTheme.Values.viewBarControlStyle
                Layout.topMargin: StudioTheme.Values.marginTopBottom
                checked: root.rootView ? root.rootView.liveUpdate : false
                onToggled: root.rootView.liveUpdate = checked
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
