// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme as StudioTheme

T.TextField {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    signal searchChanged(string searchText)

    property bool empty: control.text === ""

    function isEmpty() {
        return control.text === ""
    }

    width: control.style.controlSize.width
    height: control.style.controlSize.height

    horizontalAlignment: Qt.AlignLeft
    verticalAlignment: Qt.AlignVCenter

    leftPadding: 32
    rightPadding: 30

    font.pixelSize: control.style.baseFontSize

    color: control.style.text.idle
    selectionColor: control.style.text.selection
    selectedTextColor: control.style.text.selectedText
    placeholderTextColor: control.style.text.placeholder

    placeholderText: qsTr("Search")

    selectByMouse: true
    readOnly: false
    hoverEnabled: true
    clip: true

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape && event.modifiers === Qt.NoModifier) {
            control.clear()
            event.accepted = true
        }
    }

    Text {
        id: placeholder
        x: control.leftPadding
        y: control.topPadding
        width: control.width - (control.leftPadding + control.rightPadding)
        height: control.height - (control.topPadding + control.bottomPadding)

        text: control.placeholderText
        font: control.font
        color: control.placeholderTextColor
        verticalAlignment: control.verticalAlignment
        visible: !control.length && !control.preeditText
                 && (!control.activeFocus || control.horizontalAlignment !== Qt.AlignHCenter)
        elide: Text.ElideRight
        renderType: control.renderType
    }

    background: Rectangle {
        id: textFieldBackground
        color: control.style.background.idle
        border.color: control.style.border.idle
        border.width: control.style.borderWidth
        radius: control.style.radius

        /* TODO: Lets do this when the widget controls are removed so they remain consistent
        Behavior on color {
            ColorAnimation {
                duration: StudioTheme.Values.hoverDuration
                easing.type: StudioTheme.Values.hoverEasing
            }
        }
        */
    }

    onTextChanged: control.searchChanged(control.text)

    T.Label {
        id: searchIcon
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
        visible: control.text !== ""
        color: xMouseArea.containsMouse ? control.style.panel.background : "transparent"

        T.Label {
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
            onClicked: control.text = ""
        }
    }

    states: [
        State {
            name: "default"
            when: control.enabled && !control.hovered && !control.activeFocus
            PropertyChanges {
                target: textFieldBackground
                color: control.style.background.idle
                border.color: control.style.border.idle
            }
            PropertyChanges {
                target: control
                placeholderTextColor: control.style.text.placeholder
            }
            PropertyChanges {
                target: searchIcon
                color: control.style.icon.idle
            }
        },
        State {
            name: "hover"
            when: control.enabled && control.hovered && !control.activeFocus
            PropertyChanges {
                target: textFieldBackground
                color: control.style.background.hover
                border.color: control.style.border.hover
            }
            PropertyChanges {
                target: control
                placeholderTextColor: control.style.text.placeholderHover
            }
            PropertyChanges {
                target: searchIcon
                color: control.style.icon.idle
            }
        },
        State {
            name: "edit"
            when: control.enabled && control.activeFocus
            PropertyChanges {
                target: textFieldBackground
                color: control.style.background.interaction
                border.color: control.style.border.interaction
            }
            PropertyChanges {
                target: control
                placeholderTextColor: control.style.text.placeholderInteraction
            }
            PropertyChanges {
                target: searchIcon
                color: control.style.icon.idle
            }
        },
        State {
            name: "disabled"
            when: !control.enabled
            PropertyChanges {
                target: control
                placeholderTextColor: control.style.text.disabled
            }
            PropertyChanges {
                target: searchIcon
                color: control.style.icon.disabled
            }
        }
    ]
}
