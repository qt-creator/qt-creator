// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import Qt.labs.platform as PlatformWidgets
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme as StudioTheme

Item {
    id: root

    implicitWidth: 300
    implicitHeight: innerRect.height + 6

    property color textColor

    signal selectItem(int itemIndex)
    signal deleteItem()

    Item {
        id: boundingRect

        anchors.centerIn: root
        width: root.width - 24
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

        Row {
            width: parent.width - threeDots.width
            leftPadding: 20

            Text {
                id: moveTool

                property StudioTheme.ControlStyle style: StudioTheme.Values.viewBarButtonStyle

                width: moveTool.style.squareControlSize.width
                height: nameHolder.height

                text: StudioTheme.Constants.dragmarks
                font.family: StudioTheme.Constants.iconFont.family
                font.pixelSize: moveTool.style.baseIconFontSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            Text {
                id: nameHolder

                text: collectionName
                font.pixelSize: StudioTheme.Values.baseFontSize
                color: textColor
                leftPadding: 5
                topPadding: 8
                rightPadding: 8
                bottomPadding: 8
                elide: Text.ElideMiddle
                verticalAlignment: Text.AlignVCenter
            }
        }

        Text {
            id: threeDots

            text: StudioTheme.Constants.more_medium
            font.family: StudioTheme.Constants.iconFont.family
            font.pixelSize: StudioTheme.Values.baseIconFontSize
            color: textColor
            anchors.right: boundingRect.right
            anchors.verticalCenter: parent.verticalCenter
            rightPadding: 12
            topPadding: nameHolder.topPadding
            bottomPadding: nameHolder.bottomPadding
            verticalAlignment: Text.AlignVCenter

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton + Qt.LeftButton
                onClicked: (event) => {
                    collectionMenu.open()
                    event.accepted = true
                }
            }
        }
    }

    PlatformWidgets.Menu {
        id: collectionMenu

        PlatformWidgets.MenuItem {
            text: qsTr("Delete")
            shortcut: StandardKey.Delete
            onTriggered: deleteDialog.open()
        }

        PlatformWidgets.MenuItem {
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
