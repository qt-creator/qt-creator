// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.TextField {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    // This property is used to indicate the global hover state
    property bool hover: (actionIndicator.hover || mouseArea.containsMouse || indicator.hover
                         || translationIndicator.hover) && control.enabled
    property bool edit: control.activeFocus

    property alias actionIndicator: actionIndicator
    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: control.style.actionIndicatorSize.width
    property real __actionIndicatorHeight: control.style.actionIndicatorSize.height

    property alias translationIndicator: translationIndicator
    property alias translationIndicatorVisible: translationIndicator.visible
    property real __translationIndicatorWidth: control.style.squareControlSize.width
    property real __translationIndicatorHeight: control.style.squareControlSize.height

    property alias indicator: indicator
    property alias indicatorVisible: indicator.visible

    property string preFocusText: ""

    property bool contextMenuAboutToShow: false

    signal rejected

    horizontalAlignment: Qt.AlignLeft
    verticalAlignment: Qt.AlignVCenter

    font.pixelSize: control.style.baseFontSize

    color: control.style.text.idle
    selectionColor: control.style.text.selection
    selectedTextColor: control.style.text.selectedText
    placeholderTextColor: control.style.text.placeholder

    readOnly: false
    selectByMouse: true
    persistentSelection: contextMenu.visible || control.focus
    clip: true

    width: control.style.controlSize.width
    height: control.style.controlSize.height
    implicitHeight: control.style.controlSize.height

    leftPadding: control.style.inputHorizontalPadding + actionIndicator.width
    rightPadding: control.style.inputHorizontalPadding + translationIndicator.width + indicator.width

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: true
        hoverEnabled: true
        propagateComposedEvents: true
        acceptedButtons: Qt.NoButton
        cursorShape: Qt.PointingHandCursor
    }

    onPressed: function(event) {
        if (event.button === Qt.RightButton)
            contextMenu.popup(control)
    }

    ContextMenu {
        id: contextMenu
        style: control.style
        __parentControl: control

        onClosed: control.forceActiveFocus()
        onAboutToShow: control.contextMenuAboutToShow = true
        onAboutToHide: control.contextMenuAboutToShow = false
    }

    onActiveFocusChanged: {
        // OtherFocusReason in this case means, if the TextField gets focus after the context menu
        // was closed due to an menu item click.
        if (control.activeFocus && control.focusReason !== Qt.OtherFocusReason)
            control.preFocusText = control.text

        if (!control.activeFocus)
            control.deselect()
    }

    onEditChanged: {
        if (control.edit)
            contextMenu.close()
    }

    onEditingFinished: control.focus = false

    ActionIndicator {
        id: actionIndicator
        style: control.style
        __parentControl: control
        x: 0
        y: 0
        width: actionIndicator.visible ? control.__actionIndicatorWidth : 0
        height: actionIndicator.visible ? control.__actionIndicatorHeight : 0
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
        x: actionIndicator.width
        width: control.width - actionIndicator.width
        height: control.height
    }

    Indicator {
        id: indicator
        style: control.style
        visible: false
        x: control.width - translationIndicator.width - indicator.width
        width: indicator.visible ? control.height : 0
        height: indicator.visible ? control.height : 0
    }

    TranslationIndicator {
        id: translationIndicator
        style: control.style
        __parentControl: control
        x: control.width - translationIndicator.width
        width: translationIndicator.visible ? __translationIndicatorWidth : 0
        height: translationIndicator.visible ? __translationIndicatorHeight : 0
    }

    states: [
        State {
            name: "default"
            when: control.enabled && !control.hover && !control.edit && !contextMenu.visible
            PropertyChanges {
                target: textFieldBackground
                color: control.style.background.idle
                border.color: control.style.border.idle
            }
            PropertyChanges {
                target: control
                color: control.style.text.idle
                placeholderTextColor: control.style.text.placeholder
            }
            PropertyChanges {
                target: mouseArea
                cursorShape: Qt.PointingHandCursor
            }
        },
        State {
            name: "globalHover"
            when: (actionIndicator.hover || translationIndicator.hover || indicator.hover)
                  && !control.edit && control.enabled && !contextMenu.visible
            PropertyChanges {
                target: textFieldBackground
                color: control.style.background.globalHover
                border.color: control.style.border.idle
            }
            PropertyChanges {
                target: control
                color: control.style.text.idle
                placeholderTextColor: control.style.text.placeholder
            }
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse && !actionIndicator.hover && !translationIndicator.hover
                  && !indicator.hover && !control.edit && control.enabled && !contextMenu.visible
            PropertyChanges {
                target: textFieldBackground
                color: control.style.background.hover
                border.color: control.style.border.hover
            }
            PropertyChanges {
                target: control
                color: control.style.text.hover
                placeholderTextColor: control.style.text.placeholder
            }
        },
        State {
            name: "edit"
            when: control.edit || contextMenu.visible
            PropertyChanges {
                target: textFieldBackground
                color: control.style.background.interaction
                border.color: control.style.border.interaction
            }
            PropertyChanges {
                target: control
                color: control.style.text.idle
                placeholderTextColor: control.style.text.placeholder
            }
            PropertyChanges {
                target: mouseArea
                cursorShape: Qt.IBeamCursor
            }
        },
        State {
            name: "disable"
            when: !control.enabled
            PropertyChanges {
                target: textFieldBackground
                color: control.style.background.disabled
                border.color: control.style.border.disabled
            }
            PropertyChanges {
                target: control
                color: control.style.text.disabled
                placeholderTextColor: control.style.text.disabled
            }
        }
    ]

    Keys.onEscapePressed: function(event) {
        event.accepted = true
        control.text = control.preFocusText
        control.rejected()
        control.focus = false
    }
}
