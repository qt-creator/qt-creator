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

T.AbstractButton {
    id: myButton

    property bool globalHover: false
    property bool hover: myButton.hovered

    property alias buttonIcon: buttonIcon.text
    property alias iconColor: buttonIcon.color
    property alias iconFont: buttonIcon.font.family
    property alias iconSize: buttonIcon.font.pixelSize
    property alias iconItalic: buttonIcon.font.italic
    property alias iconBold: buttonIcon.font.bold
    property alias iconRotation: buttonIcon.rotation
    property alias backgroundVisible: buttonBackground.visible
    property alias backgroundRadius: buttonBackground.radius

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    height: StudioTheme.Values.height
    width: StudioTheme.Values.height
    z: myButton.checked ? 10 : 3
    activeFocusOnTab: false

    onHoverChanged: {
        if (parent !== undefined && parent.hoverCallback !== undefined && myButton.enabled)
            parent.hoverCallback()
    }

    background: Rectangle {
        id: buttonBackground
        color: StudioTheme.Values.themeControlBackground
        border.color: StudioTheme.Values.themeControlOutline
        border.width: StudioTheme.Values.border
    }

    indicator: Item {
        x: 0
        y: 0
        width: myButton.width
        height: myButton.height

        T.Label {
            id: buttonIcon
            color: StudioTheme.Values.themeTextColor
            font.family: StudioTheme.Constants.iconFont.family
            font.pixelSize: StudioTheme.Values.myIconFontSize
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            anchors.fill: parent
            renderType: Text.QtRendering

            states: [
                State {
                    name: "default"
                    when: myButton.enabled && !myButton.pressed && !myButton.checked
                    PropertyChanges {
                        target: buttonIcon
                        color: StudioTheme.Values.themeIconColor
                    }
                },
                State {
                    name: "press"
                    when: myButton.enabled && myButton.pressed
                    PropertyChanges {
                        target: buttonIcon
                        color: StudioTheme.Values.themeIconColorInteraction
                    }
                },
                State {
                    name: "select"
                    when: myButton.enabled && !myButton.pressed && myButton.checked
                    PropertyChanges {
                        target: buttonIcon
                        color: StudioTheme.Values.themeIconColorSelected
                    }
                },
                State {
                    name: "disable"
                    when: !myButton.enabled
                    PropertyChanges {
                        target: buttonIcon
                        color: StudioTheme.Values.themeTextColorDisabled
                    }
                }
            ]
        }
    }

    states: [
        State {
            name: "default"
            when: myButton.enabled && !myButton.globalHover && !myButton.hover
                  && !myButton.pressed && !myButton.checked
            PropertyChanges {
                target: buttonBackground
                color: StudioTheme.Values.themeControlBackground
            }
            PropertyChanges {
                target: myButton
                z: 3
            }
        },
        State {
            name: "globalHover"
            when: myButton.globalHover && !myButton.hover && !myButton.pressed && myButton.enabled
            PropertyChanges {
                target: buttonBackground
                color: StudioTheme.Values.themeControlBackgroundGlobalHover
            }
        },
        State {
            name: "hover"
            when: myButton.hover && !myButton.pressed && myButton.enabled
            PropertyChanges {
                target: buttonBackground
                color: StudioTheme.Values.themeControlBackgroundHover
            }
        },
        State {
            name: "press"
            when: myButton.hover && myButton.pressed
            PropertyChanges {
                target: buttonBackground
                color: StudioTheme.Values.themeControlBackgroundInteraction
                border.color: StudioTheme.Values.themeInteraction
            }
            PropertyChanges {
                target: myButton
                z: 100
            }
        },
        State {
            name: "disable"
            when: !myButton.enabled
            PropertyChanges {
                target: buttonBackground
                color: StudioTheme.Values.themeControlBackgroundDisabled
                border.color: StudioTheme.Values.themeControlOutlineDisabled
            }
        }
    ]
}
