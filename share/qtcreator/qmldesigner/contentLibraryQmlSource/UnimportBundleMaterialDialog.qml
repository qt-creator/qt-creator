// Copyright (C) 2022 The Qt Company Ltd.
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
    property var targetBundleLabel // "effect" or "material"
    property var targetBundleModel

    title: qsTr("Bundle %1 might be in use").arg(root.targetBundleLabel)
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

            text: qsTr("If the %1 you are removing is in use, it might cause the project to malfunction.\n\nAre you sure you want to remove it?")
                        .arg(root.targetBundleLabel)
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
                    root.targetBundleModel.removeFromProject(root.targetBundleItem)
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
