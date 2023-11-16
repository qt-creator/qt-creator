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
    implicitHeight: wholeColumn.height

    property bool hasSelectedTarget

    property color textColor
    property var collectionModel

    property bool expanded: false

    signal selectItem(int itemIndex)
    signal deleteItem()
    signal assignToSelected()

    function toggleExpanded() {
        if (collectionListView.count > 0)
            root.expanded = !root.expanded || sourceIsSelected;
    }

    ColumnLayout {
        id: wholeColumn
        width: parent.width
        spacing: 0

        Item {
            id: boundingRect

            Layout.fillWidth: true
            Layout.preferredHeight: nameHolder.height
            Layout.leftMargin: 6
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
                    root.toggleExpanded()
                }
            }

            Rectangle {
                id: innerRect
                anchors.fill: parent
            }

            RowLayout {
                width: parent.width

                Text {
                    id: expandButton

                    property StudioTheme.ControlStyle style: StudioTheme.Values.viewBarButtonStyle

                    Layout.preferredWidth: expandButton.style.squareControlSize.width
                    Layout.preferredHeight: nameHolder.height
                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

                    text: StudioTheme.Constants.startNode
                    font.family: StudioTheme.Constants.iconFont.family
                    font.pixelSize: 6
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: textColor

                    rotation: root.expanded ? 90 : 0
                    visible: collectionListView.count > 0

                    Behavior on rotation {
                        SpringAnimation { spring: 2; damping: 0.2 }
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.RightButton | Qt.LeftButton
                        onClicked: root.toggleExpanded()
                    }
                }

                Text {
                    id: nameHolder

                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

                    text: sourceName
                    font.pixelSize: StudioTheme.Values.baseFontSize
                    color: textColor
                    leftPadding: 5
                    topPadding: 8
                    rightPadding: 8
                    bottomPadding: 8
                    elide: Text.ElideMiddle
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }

                Text {
                    id: threeDots

                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                    text: StudioTheme.Constants.more_medium
                    font.family: StudioTheme.Constants.iconFont.family
                    font.pixelSize: StudioTheme.Values.baseIconFontSize
                    color: textColor
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

        ListView {
            id: collectionListView

            Layout.fillWidth: true
            Layout.preferredHeight: root.expanded ? contentHeight : 0
            Layout.leftMargin: 6
            model: internalModels
            clip: true

            Behavior on Layout.preferredHeight {
                NumberAnimation {duration: 500}
            }

            delegate: CollectionItem {
                width: collectionListView.width
                sourceType: collectionListView.model.sourceType
                onDeleteItem: collectionListView.model.removeRow(index)
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
            onTriggered: root.assignToSelected()
        }
    }

    component Spacer: Item {
        implicitWidth: 1
        implicitHeight: StudioTheme.Values.sectionColumnSpacing
    }

    StudioControls.Dialog {
        id: deleteDialog

        title: qsTr("Deleting source")

        contentItem: ColumnLayout {
            spacing: StudioTheme.Values.sectionColumnSpacing

            Text {
                text: qsTr("Are you sure that you want to delete source \"" + sourceName + "\"?")
                color: StudioTheme.Values.themeTextColor
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                spacing: StudioTheme.Values.sectionRowSpacing

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

        contentItem: ColumnLayout {
            spacing: 2

            Text {
                text: qsTr("Previous name: " + sourceName)
                color: StudioTheme.Values.themeTextColor
            }

            Spacer {}

            Text {
                text: qsTr("New name:")
                color: StudioTheme.Values.themeTextColor
            }

            StudioControls.TextField {
                id: newNameField

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
                expanded: true
            }

            PropertyChanges {
                target: expandButton
                enabled: false
            }
        }
    ]
}
