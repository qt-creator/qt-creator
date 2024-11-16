// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import StudioTheme as StudioTheme

Rectangle {
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
                text: "Columns"
                style: StudioTheme.Values.viewBarControlStyle
                Layout.topMargin: StudioTheme.Values.marginTopBottom
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
