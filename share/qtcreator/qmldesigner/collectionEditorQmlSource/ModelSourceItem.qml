// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme as StudioTheme

Item {
    id: root

    implicitWidth: 300
    implicitHeight: wholeColumn.height + 6

    property color textColor
    property var collectionModel

    property bool expanded: false

    signal selectItem(int itemIndex)
    signal deleteItem()

    Column {
        id: wholeColumn

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
                    if (!sourceIsSelected) {
                        sourceIsSelected = true
                        event.accepted = true
                    }
                }

                onDoubleClicked: (event) => {
                    if (collectionListView.count > 0)
                       root.expanded = !root.expanded;
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
                    id: expandButton

                    property StudioTheme.ControlStyle style: StudioTheme.Values.viewBarButtonStyle

                    width: expandButton.style.squareControlSize.width
                    height: nameHolder.height

                    text: StudioTheme.Constants.startNode
                    font.family: StudioTheme.Constants.iconFont.family
                    font.pixelSize: expandButton.style.baseIconFontSize
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: textColor

                    rotation: root.expanded ? 90 : 0

                    Behavior on rotation {
                        SpringAnimation { spring: 2; damping: 0.2 }
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.RightButton + Qt.LeftButton
                        onClicked: (event) => {
                            root.expanded = !root.expanded
                            event.accepted = true
                        }
                    }
                    visible: collectionListView.count > 0
                }

                Text {
                    id: nameHolder

                    text: sourceName
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
                        collectionMenu.popup()
                        event.accepted = true
                    }
                }
            }
        }

        ListView {
            id: collectionListView

            width: parent.width
            height: root.expanded ? contentHeight : 0
            model: collections
            clip: true

            Behavior on height {
                NumberAnimation {duration: 500}
            }

            delegate: CollectionItem {
                width: parent.width
                onDeleteItem: root.model.removeRow(index)
            }
        }
    }

    StudioControls.Menu {
        id: collectionMenu

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

        title: qsTr("Deleting source")

        contentItem: Column {
            spacing: 2

            Text {
                text: qsTr("Are you sure that you want to delete source \"" + sourceName + "\"?")
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

                    text: qsTr("Delete")
                    onClicked: root.deleteItem(index)
                }

                HelperWidgets.Button {
                    text: qsTr("Cancel")
                    onClicked: deleteDialog.reject()
                }
            }
        }
    }

    StudioControls.Dialog {
        id: renameDialog

        title: qsTr("Rename source")

        onAccepted: {
            if (newNameField.text !== "")
                sourceName = newNameField.text
        }

        onOpened: {
            newNameField.text = sourceName
        }

        contentItem: Column {
            spacing: 2

            Text {
                text: qsTr("Previous name: " + sourceName)
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

                    text: qsTr("Rename")
                    onClicked: renameDialog.accept()
                }

                HelperWidgets.Button {
                    text: qsTr("Cancel")
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
            when: !sourceIsSelected && !itemMouse.containsMouse

            PropertyChanges {
                target: innerRect
                opacity: 0.4
                color: StudioTheme.Values.themeControlBackground
            }

            PropertyChanges {
                target: root
                textColor: StudioTheme.Values.themeTextColor
            }
        },
        State {
            name: "hovered"
            when: !sourceIsSelected && itemMouse.containsMouse

            PropertyChanges {
                target: innerRect
                opacity: 0.5
                color: StudioTheme.Values.themeControlBackgroundHover
            }

            PropertyChanges {
                target: root
                textColor: StudioTheme.Values.themeTextColor
            }
        },
        State {
            name: "selected"
            when: sourceIsSelected

            PropertyChanges {
                target: innerRect
                opacity: 0.6
                color: StudioTheme.Values.themeControlBackgroundInteraction
            }

            PropertyChanges {
                target: root
                textColor: StudioTheme.Values.themeIconColorSelected
            }
        }
    ]
}
