// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls as QC
import QtQuickDesignerTheme 1.0
import StudioTheme 1.0 as StudioTheme

Item {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property alias text: searchFilterText.text

    signal searchChanged(string searchText)

    function clear() {
        searchFilterText.text = ""
    }

    function isEmpty() {
        return searchFilterText.text === ""
    }

    implicitWidth: searchFilterText.width
    implicitHeight: searchFilterText.height

    QC.TextField {
        id: searchFilterText
        placeholderText: qsTr("Search")
        placeholderTextColor: control.style.text.placeholder
        color: control.style.text.idle
        selectionColor: control.style.text.selection
        selectedTextColor: control.style.text.selectedText
        background: Rectangle {
            id: textFieldBackground
            color: control.style.background.idle
            border.color: control.style.border.idle
            border.width: control.style.borderWidth
            radius: control.style.radius

            // lets do this when the widget controls are removed so they remain consistent
//            Behavior on color {
//                ColorAnimation {
//                    duration: StudioTheme.Values.hoverDuration
//                    easing.type: StudioTheme.Values.hoverEasing
//                }
//           }
        }

        height: control.style.controlSize.height
        leftPadding: 32
        rightPadding: 30
        topPadding: 6
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 5
        anchors.rightMargin: 5
        selectByMouse: true
        hoverEnabled: true

        onTextChanged: control.searchChanged(text)

        QC.Label {
            text: StudioTheme.Constants.search_small
            font.family: StudioTheme.Constants.iconFont.family
            font.pixelSize: control.style.baseIconFontSize
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            color: control.style.icon.idle
        }

        Rectangle { // x button
            width: 16
            height: 15
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.verticalCenter: parent.verticalCenter
            visible: searchFilterText.text !== ""
            color: xMouseArea.containsMouse ? control.style.panel.background : "transparent"

            QC.Label {
                text: StudioTheme.Constants.close_small
                font.family: StudioTheme.Constants.iconFont.family
                font.pixelSize: control.style.baseIconFontSize
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                anchors.centerIn: parent
                color: control.style.icon.idle
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
                    color: control.style.background.idle
                    border.color: control.style.border.idle
                }
                PropertyChanges {
                    target: searchFilterText
                    placeholderTextColor: control.style.text.placeholder
                }
            },
            State {
                name: "hover"
                when: control.enabled && searchFilterText.hovered && !searchFilterText.activeFocus
                PropertyChanges {
                    target: textFieldBackground
                    color: control.style.background.hover
                    border.color: control.style.border.hover
                }

                PropertyChanges {
                    target: searchFilterText
                    placeholderTextColor: control.style.text.placeholderHover
                }
            },
            State {
                name: "edit"
                when: searchFilterText.activeFocus
                PropertyChanges {
                    target: textFieldBackground
                    color: control.style.background.interaction
                    border.color: control.style.border.interaction
                }
                PropertyChanges {
                    target: searchFilterText
                    placeholderTextColor: control.style.text.placeholderInteraction
                }
            }
        ]
    }
}
