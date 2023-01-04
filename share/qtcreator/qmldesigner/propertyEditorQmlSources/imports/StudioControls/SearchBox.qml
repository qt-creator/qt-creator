// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuickDesignerTheme 1.0
import StudioTheme 1.0 as StudioTheme

Item {
    id: root

    property alias text: searchFilterText.text

    signal searchChanged(string searchText);

    function clear()
    {
        searchFilterText.text = "";
    }

    function isEmpty()
    {
        return searchFilterText.text === "";
    }

    implicitWidth: searchFilterText.width
    implicitHeight: searchFilterText.height

    TextField {
        id: searchFilterText

        placeholderText: qsTr("Search")
        placeholderTextColor: StudioTheme.Values.themePlaceholderTextColor
        color: StudioTheme.Values.themeTextColor
        selectionColor: StudioTheme.Values.themeTextSelectionColor
        selectedTextColor: StudioTheme.Values.themeTextSelectedTextColor
        background: Rectangle {
            id: textFieldBackground
            color: StudioTheme.Values.themeControlBackground
            border.color: StudioTheme.Values.themeControlOutline
            border.width: StudioTheme.Values.border

            Behavior on color {
                ColorAnimation {
                    duration: StudioTheme.Values.hoverDuration
                    easing.type: StudioTheme.Values.hoverEasing
                }
            }
        }

        height: StudioTheme.Values.defaultControlHeight

        leftPadding: 32
        rightPadding: 30
        topPadding: 6
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 5
        anchors.rightMargin: 5
        selectByMouse: true
        hoverEnabled: true

        onTextChanged: root.searchChanged(text)

        Label {
            text: StudioTheme.Constants.search
            font.family: StudioTheme.Constants.iconFont.family
            font.pixelSize: StudioTheme.Values.myIconFontSize
            anchors.left: parent.left
            anchors.leftMargin: 7
            anchors.verticalCenter: parent.verticalCenter
            color: StudioTheme.Values.themeIconColor
        }

        Rectangle { // x button
            width: 16
            height: 15
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.verticalCenter: parent.verticalCenter
            visible: searchFilterText.text !== ""
            color: xMouseArea.containsMouse ? StudioTheme.Values.themePanelBackground
                                            : "transparent"

            Label {
                text: StudioTheme.Constants.closeCross
                font.family: StudioTheme.Constants.iconFont.family
                font.pixelSize: StudioTheme.Values.myIconFontSize
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                anchors.centerIn: parent
                color: StudioTheme.Values.themeIconColor
            }

            MouseArea {
                id: xMouseArea
                hoverEnabled: true
                anchors.fill: parent
                onClicked: searchFilterText.text = ""
            }
        }

        states: [
            State {
                name: "default"
                when: !searchFilterText.hovered && !searchFilterText.activeFocus
                PropertyChanges {
                    target: textFieldBackground
                    color: StudioTheme.Values.themeControlBackground
                    border.color: StudioTheme.Values.themeControlOutline
                }
                PropertyChanges {
                    target: searchFilterText
                    placeholderTextColor: StudioTheme.Values.themePlaceholderTextColor
                }
            },
            State {
                name: "hover"
                when: root.enabled && searchFilterText.hovered && !searchFilterText.activeFocus
                PropertyChanges {
                    target: textFieldBackground
                    color: StudioTheme.Values.themeControlBackgroundHover
                    border.color: StudioTheme.Values.themeControlOutline
                }

                PropertyChanges {
                    target: searchFilterText
                    placeholderTextColor: StudioTheme.Values.themePlaceholderTextColor
                }
            },
            State {
                name: "edit"
                when: searchFilterText.activeFocus
                PropertyChanges {
                    target: textFieldBackground
                    color: StudioTheme.Values.themeControlBackgroundInteraction
                    border.color: StudioTheme.Values.themeControlOutlineInteraction
                }
                PropertyChanges {
                    target: searchFilterText
                    placeholderTextColor: StudioTheme.Values.themePlaceholderTextColorInteraction
                }
            }
        ]
    }
}
