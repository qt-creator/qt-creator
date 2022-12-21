// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuickDesignerTheme
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Dialog {
    id: root

    title: qsTr("Bundle material might be in use")
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    implicitWidth: 300
    modal: true

    property var targetBundleMaterial

    contentItem: Column {
        spacing: 20
        width: parent.width

        Text {
            id: folderNotEmpty

            text: qsTr("If the material you are removing is in use, it might cause the project to malfunction.\n\nAre you sure you want to remove the material?")
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
                    materialsModel.removeFromProject(root.targetBundleMaterial)
                    root.accept()
                }
            }

            Button {
                text: qsTr("Cancel")
                onClicked: root.reject()
            }
        }
    }

    onOpened: folderNotEmpty.forceActiveFocus()
}
