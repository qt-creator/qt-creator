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

Item {
    id: translationIndicator

    property Item myControl

    property bool hover: translationIndicatorMouseArea.containsMouse && translationIndicator.enabled
    property bool pressed: translationIndicatorMouseArea.pressed
    property bool checked: false

    signal clicked

    Rectangle {
        id: translationIndicatorBackground
        color: StudioTheme.Values.themeControlBackground
        border.color: StudioTheme.Values.themeControlOutline
        border.width: StudioTheme.Values.border

        anchors.centerIn: parent

        width: matchParity(translationIndicator.height, StudioTheme.Values.smallRectWidth)
        height: matchParity(translationIndicator.height, StudioTheme.Values.smallRectWidth)

        function matchParity(root, value) {
            var v = Math.round(value)

            if (root % 2 == 0)
                return (v % 2 == 0) ? v : v - 1
            else
                return (v % 2 == 0) ? v - 1 : v
        }

        MouseArea {
            id: translationIndicatorMouseArea
            anchors.fill: parent
            hoverEnabled: true
            onPressed: function(mouse) { mouse.accepted = true }
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

        states: [
            State {
                name: "default"
                when: translationIndicator.enabled && !translationIndicator.pressed
                      && !translationIndicator.checked
                PropertyChanges {
                    target: translationIndicatorIcon
                    color: StudioTheme.Values.themeIconColor
                }
            },
            State {
                name: "press"
                when: translationIndicator.enabled && translationIndicator.pressed
                PropertyChanges {
                    target: translationIndicatorIcon
                    color: StudioTheme.Values.themeIconColorInteraction
                }
            },
            State {
                name: "check"
                when: translationIndicator.enabled && !translationIndicator.pressed
                      && translationIndicator.checked
                PropertyChanges {
                    target: translationIndicatorIcon
                    color: StudioTheme.Values.themeIconColorSelected
                }
            },
            State {
                name: "disable"
                when: !myControl.enabled
                PropertyChanges {
                    target: translationIndicatorIcon
                    color: StudioTheme.Values.themeTextColorDisabled
                }
            }
        ]
    }

    states: [
        State {
            name: "default"
            when: myControl.enabled && !translationIndicator.hover
                  && !translationIndicator.pressed && !myControl.hover
                  && !myControl.edit && !translationIndicator.checked
            PropertyChanges {
                target: translationIndicatorBackground
                color: StudioTheme.Values.themeControlBackground
                border.color: StudioTheme.Values.themeControlOutline
            }
        },
        State {
            name: "globalHover"
            when: myControl.hover && !translationIndicator.hover
            PropertyChanges {
                target: translationIndicatorBackground
                color: StudioTheme.Values.themeControlBackgroundGlobalHover
                border.color: StudioTheme.Values.themeControlOutline
            }
        },
        State {
            name: "hover"
            when: translationIndicator.hover && !translationIndicator.pressed
            PropertyChanges {
                target: translationIndicatorBackground
                color: StudioTheme.Values.themeControlBackgroundHover
                border.color: StudioTheme.Values.themeControlOutline
            }
        },
        State {
            name: "press"
            when: translationIndicator.hover && translationIndicator.pressed
            PropertyChanges {
                target: translationIndicatorBackground
                color: StudioTheme.Values.themeControlBackgroundInteraction
                border.color: StudioTheme.Values.themeControlOutlineInteraction
            }
        },
        State {
            name: "disable"
            when: !myControl.enabled
            PropertyChanges {
                target: translationIndicatorBackground
                color: StudioTheme.Values.themeControlBackgroundDisabled
                border.color: StudioTheme.Values.themeControlOutlineDisabled
            }
        }
    ]
}
