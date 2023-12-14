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
    implicitHeight: boundingRect.height + 3

    property color textColor
    property string sourceType
    property bool hasSelectedTarget

    signal selectItem(int itemIndex)
    signal deleteItem()

    Item {
        id: boundingRect

        width: parent.width
        height: itemLayout.height
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
            id: itemLayout

            width: parent.width

            Text {
                id: nameHolder

                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                Layout.leftMargin: StudioTheme.Values.collectionItemTextSideMargin
                Layout.topMargin: StudioTheme.Values.collectionItemTextMargin
                Layout.bottomMargin: StudioTheme.Values.collectionItemTextMargin

                text: collectionName
                font.pixelSize: StudioTheme.Values.baseFontSize
                color: root.textColor
                topPadding: StudioTheme.Values.collectionItemTextPadding
                bottomPadding: StudioTheme.Values.collectionItemTextPadding
                elide: Text.ElideMiddle
                verticalAlignment: Text.AlignVCenter
            }

            Text {
                id: threeDots

                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                Layout.topMargin: StudioTheme.Values.collectionItemTextMargin
                Layout.bottomMargin: StudioTheme.Values.collectionItemTextMargin
                Layout.rightMargin: StudioTheme.Values.collectionItemTextSideMargin

                text: StudioTheme.Constants.more_medium
                font.family: StudioTheme.Constants.iconFont.family
                font.pixelSize: StudioTheme.Values.baseIconFontSize
                color: root.textColor
                padding: StudioTheme.Values.collectionItemTextPadding

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

        StudioControls.MenuItem {
            text: qsTr("Assign to the selected node")
            enabled: root.hasSelectedTarget
            onTriggered: rootView.assignCollectionToSelectedNode(collectionName)
        }
    }

    component Spacer: Item {
        implicitWidth: 1
        implicitHeight: StudioTheme.Values.columnGap
    }

    StudioControls.Dialog {
        id: deleteDialog

        title: qsTr("Deleting the model")
        clip: true
        implicitWidth: 300

        contentItem: ColumnLayout {
            spacing: 2

            Text {
                Layout.fillWidth: true

                wrapMode: Text.WordWrap
                color: StudioTheme.Values.themeTextColor
                text: {
                    if (root.sourceType === "json") {
                        qsTr("Are you sure that you want to delete model \"%1\"?"
                             + "\nThe model will be deleted permanently.").arg(collectionName)
                    } else if (root.sourceType === "csv") {
                        qsTr("Are you sure that you want to delete model \"%1\"?"
                             + "\nThe model will be removed from the project "
                             + "but the file will not be deleted.").arg(collectionName)
                    }
                }
            }

            Spacer {}

            RowLayout {
                spacing: StudioTheme.Values.sectionRowSpacing
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                HelperWidgets.Button {
                    text: qsTr("Delete")
                    onClicked: root.deleteItem()
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

        title: qsTr("Rename model")

        onAccepted: {
            if (newNameField.text !== "")
                collectionName = newNameField.text
        }

        onOpened: {
            newNameField.text = collectionName
        }

        contentItem: ColumnLayout {
            spacing: 2

            Text {
                text: qsTr("Previous name: " + collectionName)
                color: StudioTheme.Values.themeTextColor
            }

            Spacer {}

            Text {
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                text: qsTr("New name:")
                color: StudioTheme.Values.themeTextColor
            }

            StudioControls.TextField {
                id: newNameField

                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                Layout.fillWidth: true

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

            Spacer {}

            RowLayout {
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                spacing: StudioTheme.Values.sectionRowSpacing

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

    RegularExpressionValidator {
        id: newNameValidator
        regularExpression: /^\w+$/
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
                color: StudioTheme.Values.themeIconColorSelected
            }

            PropertyChanges {
                target: root
                textColor: StudioTheme.Values.themeTextSelectedTextColor
            }
        }
    ]
}
