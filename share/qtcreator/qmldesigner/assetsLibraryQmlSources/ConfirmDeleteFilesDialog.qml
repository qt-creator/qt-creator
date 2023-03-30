// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme
import StudioControls as StudioControls
import AssetsLibraryBackend

StudioControls.Dialog {
    id: root
    title: qsTr("Confirm Delete Files")
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    implicitWidth: 350
    modal: true

    required property var files

    contentItem: Column {
        spacing: 20
        width: parent.width

        Text {
            id: confirmDeleteText

            text: root.files && root.files.length
                    ? qsTr("Some files might be in use. Delete anyway?")
                    : ""
            color: StudioTheme.Values.themeTextColor
            wrapMode: Text.WordWrap
            width: root.width
            leftPadding: 10
            rightPadding: 10

            Keys.onEnterPressed: btnDelete.onClicked()
            Keys.onReturnPressed: btnDelete.onClicked()
        }

        Loader {
            id: fileListLoader
            sourceComponent: null
            width: parent.width - 20
            x: 10
            height: 50
        }

        StudioControls.CheckBox {
            id: dontAskAgain
            text: qsTr("Do not ask this again")
            actionIndicatorVisible: false
            width: parent.width
            x: 10
        }

        Row {
            anchors.right: parent.right

            HelperWidgets.Button {
                id: btnDelete

                text: qsTr("Delete")
                onClicked: {
                    AssetsLibraryBackend.assetsModel.deleteFiles(root.files, dontAskAgain.checked)
                    root.accept()
                }
            }

            HelperWidgets.Button {
                text: qsTr("Cancel")
                onClicked: root.reject()
            }
        }
    }

    onOpened: {
        fileListLoader.sourceComponent = null
        fileListLoader.sourceComponent = fileListComponent

        confirmDeleteText.forceActiveFocus()
    }

    onClosed: fileListLoader.sourceComponent = null

    Component {
        id: fileListComponent

        ListView {
            id: filesListView
            model: root.files
            boundsBehavior: Flickable.StopAtBounds
            clip: true

            ScrollBar.vertical: HelperWidgets.VerticalScrollBar {
                id: verticalScrollBar
                scrollBarVisible: filesListView.contentHeight > filesListView.height
            }

            delegate: Text {
                elide: Text.ElideLeft
                text: model.modelData.replace(AssetsLibraryBackend.assetsModel.currentProjectDirPath(), "")
                color: StudioTheme.Values.themeTextColor
                width: parent.width - (verticalScrollBar.scrollBarVisible ? verticalScrollBar.width : 0)
            }
        } // ListView
    } // Component
}
