// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme as StudioTheme
import CollectionEditorBackend

Item {
    id: root

    implicitWidth: 300
    implicitHeight: boundingRect.height + 3

    property color textColor
    readonly property string name: collectionName ?? ""
    readonly property bool isSelected: collectionIsSelected
    readonly property int id: index

    function rename(newName) {
        collectionName = newName
    }

    signal selectItem(int itemIndex)
    signal deleteItem()
    signal contextMenuRequested()

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
                    onClicked: root.contextMenuRequested()
                }
            }
        }
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
