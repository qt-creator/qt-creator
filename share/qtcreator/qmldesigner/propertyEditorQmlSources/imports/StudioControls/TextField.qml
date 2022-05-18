/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

T.TextField {
    id: root

    property alias actionIndicator: actionIndicator
    property alias translationIndicator: translationIndicator

    // This property is used to indicate the global hover state
    property bool hover: (actionIndicator.hover || mouseArea.containsMouse
                         || translationIndicator.hover) && root.enabled
    property bool edit: root.activeFocus

    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: StudioTheme.Values.actionIndicatorWidth
    property real __actionIndicatorHeight: StudioTheme.Values.actionIndicatorHeight

    property alias translationIndicatorVisible: translationIndicator.visible
    property real __translationIndicatorWidth: StudioTheme.Values.translationIndicatorWidth
    property real __translationIndicatorHeight: StudioTheme.Values.translationIndicatorHeight

    property string preFocusText: ""

    horizontalAlignment: Qt.AlignLeft
    verticalAlignment: Qt.AlignVCenter

    font.pixelSize: StudioTheme.Values.myFontSize

    color: StudioTheme.Values.themeTextColor
    selectionColor: StudioTheme.Values.themeTextSelectionColor
    selectedTextColor: StudioTheme.Values.themeTextSelectedTextColor
    placeholderTextColor: StudioTheme.Values.themePlaceholderTextColor

    readOnly: false
    selectByMouse: true
    persistentSelection: focus // QTBUG-73807
    clip: true

    width: StudioTheme.Values.defaultControlWidth
    height: StudioTheme.Values.defaultControlHeight
    implicitHeight: StudioTheme.Values.defaultControlHeight

    leftPadding: StudioTheme.Values.inputHorizontalPadding + actionIndicator.width
    rightPadding: StudioTheme.Values.inputHorizontalPadding + translationIndicator.width

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: true
        hoverEnabled: true
        propagateComposedEvents: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: Qt.PointingHandCursor
        onPressed: function(mouse) {
            if (mouse.button === Qt.RightButton)
                contextMenu.popup(root)

            mouse.accepted = false
        }
    }

    onPersistentSelectionChanged: {
        if (!persistentSelection)
            root.deselect()
    }

    ContextMenu {
        id: contextMenu
        myTextEdit: root
    }

    onActiveFocusChanged: {
        if (root.activeFocus)
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
            when: root.enabled && !root.hover && !root.edit
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
            when: (actionIndicator.hover || translationIndicator.hover) && !root.edit
                  && root.enabled
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
                  && !root.edit && root.enabled
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
            when: root.edit
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

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Escape) {
            root.text = root.preFocusText
            root.focus = false
        }
    }
}
