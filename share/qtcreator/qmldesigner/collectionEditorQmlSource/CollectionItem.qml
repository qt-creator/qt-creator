// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme as StudioTheme

Item {
    id: root

    implicitWidth: 300
    implicitHeight: innerRect.height + 3

    property color textColor

    signal selectItem(int itemIndex)
    signal deleteItem()

    Item {
        id: boundingRect

        anchors.centerIn: root
        width: parent.width
        height: nameHolder.height
        clip: true

        MouseArea {
            id: itemMouse

            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            propagateComposedEvents: true
            hoverEnabled: true
            onClicked: (event) => {
                if (!collectionIsSelected) {
                    collectionIsSelected = true
                    event.accepted = true
                }
            }
        }

        Rectangle {
            id: innerRect
            anchors.fill: parent
        }

        RowLayout {
            width: parent.width

            Text {
                id: moveTool

                property StudioTheme.ControlStyle style: StudioTheme.Values.viewBarButtonStyle

                Layout.preferredWidth: moveTool.style.squareControlSize.width
                Layout.preferredHeight: nameHolder.height
                Layout.leftMargin: 12
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

                text: StudioTheme.Constants.dragmarks
                font.family: StudioTheme.Constants.iconFont.family
                font.pixelSize: moveTool.style.baseIconFontSize
                color: root.textColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            Text {
                id: nameHolder

                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

                text: collectionName
                font.pixelSize: StudioTheme.Values.baseFontSize
                color: root.textColor
                leftPadding: 5
                topPadding: 8
                rightPadding: 8
                bottomPadding: 8
                elide: Text.ElideMiddle
                verticalAlignment: Text.AlignVCenter
            }

            Text {
                id: threeDots

                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                text: StudioTheme.Constants.more_medium
                font.family: StudioTheme.Constants.iconFont.family
                font.pixelSize: StudioTheme.Values.baseIconFontSize
                color: root.textColor
                rightPadding: 12
                topPadding: nameHolder.topPadding
                bottomPadding: nameHolder.bottomPadding
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton | Qt.LeftButton
                    onClicked: collectionMenu.popup()
                }
            }
        }
    }

    StudioControls.Menu {
        id: collectionMenu

        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        StudioControls.MenuItem {
            text: qsTr("Delete")
            shortcut: StandardKey.Delete
            onTriggered: deleteDialog.open()
        }

        StudioControls.MenuItem {
            text: qsTr("Rename")
            shortcut: StandardKey.Replace
            onTriggered: renameDialog.open()
        }
    }

    StudioControls.Dialog {
        id: deleteDialog

        title: qsTr("Deleting whole collection")

        contentItem: Column {
            spacing: 2

            Text {
                text: qsTr("Are you sure that you want to delete collection \"" + collectionName + "\"?")
                color: StudioTheme.Values.themeTextColor
            }

            Item { // spacer
                width: 1
                height: 20
            }

            Row {
                anchors.right: parent.right
                spacing: 10

                HelperWidgets.Button {
                    id: btnDelete
                    anchors.verticalCenter: parent.verticalCenter

                    text: qsTr("Delete")
                    onClicked: root.deleteItem(index)
                }

                HelperWidgets.Button {
                    text: qsTr("Cancel")
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: deleteDialog.reject()
                }
            }
        }
    }

    StudioControls.Dialog {
        id: renameDialog

        title: qsTr("Rename collection")

        onAccepted: {
            if (newNameField.text !== "")
                collectionName = newNameField.text
        }

        onOpened: {
            newNameField.text = collectionName
        }

        contentItem: Column {
            spacing: 2

            Text {
                text: qsTr("Previous name: " + collectionName)
                color: StudioTheme.Values.themeTextColor
            }

            Row {
                spacing: 10
                Text {
                    text: qsTr("New name:")
                    color: StudioTheme.Values.themeTextColor
                }

                StudioControls.TextField {
                    id: newNameField

                    anchors.verticalCenter: parent.verticalCenter
                    actionIndicator.visible: false
                    translationIndicator.visible: false
                    validator: newNameValidator

                    Keys.onEnterPressed: renameDialog.accept()
                    Keys.onReturnPressed: renameDialog.accept()
                    Keys.onEscapePressed: renameDialog.reject()

                    onTextChanged: {
                        btnRename.enabled = newNameField.text !== ""
                    }
                }
            }

            Item { // spacer
                width: 1
                height: 20
            }

            Row {
                anchors.right: parent.right
                spacing: 10

                HelperWidgets.Button {
                    id: btnRename

                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Rename")
                    onClicked: renameDialog.accept()
                }

                HelperWidgets.Button {
                    text: qsTr("Cancel")
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: renameDialog.reject()
                }
            }
        }
    }

    HelperWidgets.RegExpValidator {
        id: newNameValidator
        regExp: /^\w+$/
    }

    states: [
        State {
            name: "default"
            when: !collectionIsSelected && !itemMouse.containsMouse

            PropertyChanges {
                target: innerRect
                opacity: 0.6
                color: StudioTheme.Values.themeControlBackground
            }

            PropertyChanges {
                target: root
                textColor: StudioTheme.Values.themeTextColor
            }
        },
        State {
            name: "hovered"
            when: !collectionIsSelected && itemMouse.containsMouse

            PropertyChanges {
                target: innerRect
                opacity: 0.8
                color: StudioTheme.Values.themeControlBackgroundHover
            }

            PropertyChanges {
                target: root
                textColor: StudioTheme.Values.themeTextColor
            }
        },
        State {
            name: "selected"
            when: collectionIsSelected

            PropertyChanges {
                target: innerRect
                opacity: 1
                color: StudioTheme.Values.themeControlBackgroundInteraction
            }

            PropertyChanges {
                target: root
                textColor: StudioTheme.Values.themeIconColorSelected
            }
        }
    ]
}
