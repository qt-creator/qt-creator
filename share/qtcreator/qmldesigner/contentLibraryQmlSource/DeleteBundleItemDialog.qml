// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import ContentLibraryBackend

StudioControls.Dialog {
    id: root

    property var targetBundleItem
    property var targetBundleModel
    property string targetBundleLabel // "effect" or "material"

    title: qsTr("Remove bundle %1").arg(root.targetBundleLabel)
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    implicitWidth: 300
    modal: true

    onOpened: warningText.forceActiveFocus()

    contentItem: Column {
        spacing: 20
        width: parent.width

        Text {
            id: warningText

            text: qsTr("Are you sure you? The action cannot be undone")
            color: StudioTheme.Values.themeTextColor
            wrapMode: Text.WordWrap
            anchors.right: parent.right
            anchors.left: parent.left
            leftPadding: 10
            rightPadding: 10

            Keys.onEnterPressed: btnRemove.onClicked()
            Keys.onReturnPressed: btnRemove.onClicked()
        }

        Row {
            anchors.right: parent.right
            Button {
                id: btnRemove

                text: qsTr("Remove")

                onClicked: {
                    ContentLibraryBackend.userModel.removeFromContentLib(root.targetBundleItem)
                    root.accept()
                }
            }

            Button {
                text: qsTr("Cancel")
                onClicked: root.reject()
            }
        }
    }
}
