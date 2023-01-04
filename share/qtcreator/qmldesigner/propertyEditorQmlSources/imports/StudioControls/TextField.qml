// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

T.TextField {
    id: root

    // This property is used to indicate the global hover state
    property bool hover: (actionIndicator.hover || mouseArea.containsMouse || indicator.hover
                         || translationIndicator.hover) && root.enabled
    property bool edit: root.activeFocus

    property alias actionIndicator: actionIndicator
    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: StudioTheme.Values.actionIndicatorWidth
    property real __actionIndicatorHeight: StudioTheme.Values.actionIndicatorHeight

    property alias translationIndicator: translationIndicator
    property alias translationIndicatorVisible: translationIndicator.visible
    property real __translationIndicatorWidth: StudioTheme.Values.translationIndicatorWidth
    property real __translationIndicatorHeight: StudioTheme.Values.translationIndicatorHeight

    property alias indicator: indicator
    property alias indicatorVisible: indicator.visible

    property string preFocusText: ""

    property bool contextMenuAboutToShow: false

    signal rejected

    horizontalAlignment: Qt.AlignLeft
    verticalAlignment: Qt.AlignVCenter

    font.pixelSize: StudioTheme.Values.myFontSize

    color: StudioTheme.Values.themeTextColor
    selectionColor: StudioTheme.Values.themeTextSelectionColor
    selectedTextColor: StudioTheme.Values.themeTextSelectedTextColor
    placeholderTextColor: StudioTheme.Values.themePlaceholderTextColor

    readOnly: false
    selectByMouse: true
    persistentSelection: contextMenu.visible || root.focus
    clip: true

    width: StudioTheme.Values.defaultControlWidth
    height: StudioTheme.Values.defaultControlHeight
    implicitHeight: StudioTheme.Values.defaultControlHeight

    leftPadding: StudioTheme.Values.inputHorizontalPadding + actionIndicator.width
    rightPadding: StudioTheme.Values.inputHorizontalPadding + translationIndicator.width + indicator.width

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
            contextMenu.popup(root)
    }

    ContextMenu {
        id: contextMenu
        myTextEdit: root

        onClosed: root.forceActiveFocus()
        onAboutToShow: root.contextMenuAboutToShow = true
        onAboutToHide: root.contextMenuAboutToShow = false
    }

    onActiveFocusChanged: {
        // OtherFocusReason in this case means, if the TextField gets focus after the context menu
        // was closed due to an menu item click.
        if (root.activeFocus && root.focusReason !== Qt.OtherFocusReason)
            root.preFocusText = root.text
    }

    onEditChanged: {
        if (root.edit)
            contextMenu.close()
    }

    onEditingFinished: root.focus = false

    ActionIndicator {
        id: actionIndicator
        myControl: root
        x: 0
        y: 0
        width: actionIndicator.visible ? root.__actionIndicatorWidth : 0
        height: actionIndicator.visible ? root.__actionIndicatorHeight : 0
    }

    Text {
        id: placeholder
        x: root.leftPadding
        y: root.topPadding
        width: root.width - (root.leftPadding + root.rightPadding)
        height: root.height - (root.topPadding + root.bottomPadding)

        text: root.placeholderText
        font: root.font
        color: root.placeholderTextColor
        verticalAlignment: root.verticalAlignment
        visible: !root.length && !root.preeditText
                 && (!root.activeFocus || root.horizontalAlignment !== Qt.AlignHCenter)
        elide: Text.ElideRight
        renderType: root.renderType
    }

    background: Rectangle {
        id: textFieldBackground
        color: StudioTheme.Values.themeControlBackground
        border.color: StudioTheme.Values.themeControlOutline
        border.width: StudioTheme.Values.border
        x: actionIndicator.width
        width: root.width - actionIndicator.width
        height: root.height
    }

    Indicator {
        id: indicator
        visible: false
        x: root.width - translationIndicator.width - indicator.width
        width: indicator.visible ? root.height : 0
        height: indicator.visible ? root.height : 0
    }

    TranslationIndicator {
        id: translationIndicator
        myControl: root
        x: root.width - translationIndicator.width
        width: translationIndicator.visible ? __translationIndicatorWidth : 0
        height: translationIndicator.visible ? __translationIndicatorHeight : 0
    }

    states: [
        State {
            name: "default"
            when: root.enabled && !root.hover && !root.edit && !contextMenu.visible
            PropertyChanges {
                target: textFieldBackground
                color: StudioTheme.Values.themeControlBackground
                border.color: StudioTheme.Values.themeControlOutline
            }
            PropertyChanges {
                target: root
                color: StudioTheme.Values.themeTextColor
                placeholderTextColor: StudioTheme.Values.themePlaceholderTextColor
            }
            PropertyChanges {
                target: mouseArea
                cursorShape: Qt.PointingHandCursor
            }
        },
        State {
            name: "globalHover"
            when: (actionIndicator.hover || translationIndicator.hover || indicator.hover)
                  && !root.edit && root.enabled && !contextMenu.visible
            PropertyChanges {
                target: textFieldBackground
                color: StudioTheme.Values.themeControlBackgroundGlobalHover
                border.color: StudioTheme.Values.themeControlOutline
            }
            PropertyChanges {
                target: root
                color: StudioTheme.Values.themeTextColor
                placeholderTextColor: StudioTheme.Values.themePlaceholderTextColor
            }
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse && !actionIndicator.hover && !translationIndicator.hover
                  && !indicator.hover && !root.edit && root.enabled && !contextMenu.visible
            PropertyChanges {
                target: textFieldBackground
                color: StudioTheme.Values.themeControlBackgroundHover
                border.color: StudioTheme.Values.themeControlOutline
            }
            PropertyChanges {
                target: root
                color: StudioTheme.Values.themeTextColor
                placeholderTextColor: StudioTheme.Values.themePlaceholderTextColor
            }
        },
        State {
            name: "edit"
            when: root.edit || contextMenu.visible
            PropertyChanges {
                target: textFieldBackground
                color: StudioTheme.Values.themeControlBackgroundInteraction
                border.color: StudioTheme.Values.themeControlOutlineInteraction
            }
            PropertyChanges {
                target: root
                color: StudioTheme.Values.themeTextColor
                placeholderTextColor: StudioTheme.Values.themePlaceholderTextColorInteraction
            }
            PropertyChanges {
                target: mouseArea
                cursorShape: Qt.IBeamCursor
            }
        },
        State {
            name: "disable"
            when: !root.enabled
            PropertyChanges {
                target: textFieldBackground
                color: StudioTheme.Values.themeControlBackgroundDisabled
                border.color: StudioTheme.Values.themeControlOutlineDisabled
            }
            PropertyChanges {
                target: root
                color: StudioTheme.Values.themeTextColorDisabled
                placeholderTextColor: StudioTheme.Values.themeTextColorDisabled
            }
        }
    ]

    Keys.onEscapePressed: function(event) {
        event.accepted = true
        root.text = root.preFocusText
        root.rejected()
        root.focus = false
    }
}
