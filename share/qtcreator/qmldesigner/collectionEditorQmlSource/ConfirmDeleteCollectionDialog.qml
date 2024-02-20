// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme as StudioTheme

StudioControls.Dialog {
    id: root

    required property string collectionName

    title: qsTr("Deleting the model")
    clip: true

    contentItem: ColumnLayout {
        id: deleteDialogContent // Keep the id here even if it's not used, because the dialog might lose implicitSize

        width: 300
        spacing: 2

        Text {
            Layout.fillWidth: true

            wrapMode: Text.WordWrap
            color: StudioTheme.Values.themeTextColor
            text: qsTr("Are you sure that you want to delete model \"%1\"?"
                       + "\nThe model will be deleted permanently.").arg(root.collectionName)

        }

        Item { // spacer
            implicitWidth: 1
            implicitHeight: StudioTheme.Values.columnGap
        }

        RowLayout {
            spacing: StudioTheme.Values.sectionRowSpacing
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            Layout.fillWidth: true
            Layout.preferredHeight: 40

            HelperWidgets.Button {
                text: qsTr("Delete")
                onClicked: root.accept()
            }

            HelperWidgets.Button {
                text: qsTr("Cancel")
                onClicked: root.reject()
            }
        }
    }
}
