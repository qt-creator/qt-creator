/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.12
import QtQuick.Templates 2.12 as T
import StudioTheme 1.0 as StudioTheme

Item {
    id: translationIndicator

    property Item myControl

    property bool hover: false
    property bool pressed: false
    property bool checked: false

    signal clicked

    state: "default"

    Rectangle {
        id: translationIndicatorBackground
        color: StudioTheme.Values.themeColumnBackground // TODO create extra variable, this one is used
        border.color: StudioTheme.Values.themeTranslationIndicatorBorder

        anchors.centerIn: parent

        width: matchParity(translationIndicator.height,
                           StudioTheme.Values.smallRectWidth)
        height: matchParity(translationIndicator.height,
                            StudioTheme.Values.smallRectWidth)

        function matchParity(root, value) {
            // TODO maybe not necessary
            var v = Math.round(value)

            if (root % 2 == 0)
                // even
                return (v % 2 == 0) ? v : v - 1
            else
                // odd
                return (v % 2 == 0) ? v - 1 : v
        }

        MouseArea {
            id: translationIndicatorMouseArea
            anchors.fill: parent
            hoverEnabled: true
            onContainsMouseChanged: translationIndicator.hover = containsMouse
            onPressed: mouse.accepted = true // TODO
            onClicked: {
                translationIndicator.checked = !translationIndicator.checked
                translationIndicator.clicked()
            }
        }
    }

    T.Label {
        id: translationIndicatorIcon
        text: "tr"
        color: StudioTheme.Values.themeTextColor
        font.family: StudioTheme.Constants.font.family
        font.pixelSize: StudioTheme.Values.myIconFontSize
        font.italic: true
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        anchors.fill: parent
    }

    states: [
        State {
            name: "default"
            when: myControl.enabled && !translationIndicator.hover
                  && !translationIndicator.pressed && !myControl.hover
                  && !myControl.edit && !myControl.drag
                  && !translationIndicator.checked
            PropertyChanges {
                target: translationIndicatorBackground
                color: StudioTheme.Values.themeColumnBackground
                border.color: StudioTheme.Values.themeTranslationIndicatorBorder
            }
        },
        State {
            name: "checked"
            when: translationIndicator.checked

            PropertyChanges {
                target: translationIndicatorBackground
                color: StudioTheme.Values.themeInteraction // TODO
            }
        },
        State {
            name: "hovered"
            when: translationIndicator.hover && !translationIndicator.pressed
                  && !myControl.edit && !myControl.drag && !myControl.drag
            PropertyChanges {
                target: translationIndicatorBackground
                color: StudioTheme.Values.themeFocusDrag // TODO
            }
        },
        State {
            name: "disabled"
            when: !myControl.enabled
            PropertyChanges {
                target: translationIndicatorBackground
                color: StudioTheme.Values.themeControlBackgroundDisabled
                border.color: StudioTheme.Values.themeControlOutlineDisabled
            }
            PropertyChanges {
                target: translationIndicatorIcon
                color: StudioTheme.Values.themeTextColorDisabled
            }
        }
    ]
}
