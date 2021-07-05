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

Rectangle {
    id: spinBoxIndicator

    property T.Control myControl

    property bool hover: spinBoxIndicatorMouseArea.containsMouse && spinBoxIndicator.enabled
    property bool pressed: spinBoxIndicatorMouseArea.containsPress

    property alias iconFlip: spinBoxIndicatorIconScale.yScale

    color: StudioTheme.Values.themeControlBackground
    border.width: 0

    // This MouseArea is a workaround to avoid some hover state related bugs
    // when using the actual signal 'up.hovered'. QTBUG-74688
    MouseArea {
        id: spinBoxIndicatorMouseArea
        anchors.fill: parent
        hoverEnabled: true
        onPressed: function(mouse) {
            if (myControl.activeFocus)
                spinBoxIndicator.forceActiveFocus()

            mouse.accepted = false
        }
    }

    T.Label {
        id: spinBoxIndicatorIcon
        text: StudioTheme.Constants.upDownSquare2
        color: StudioTheme.Values.themeTextColor
        renderType: Text.NativeRendering
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: StudioTheme.Values.spinControlIconSizeMulti
        font.family: StudioTheme.Constants.iconFont.family
        anchors.fill: parent
        transform: Scale {
            id: spinBoxIndicatorIconScale
            origin.x: 0
            origin.y: spinBoxIndicatorIcon.height / 2
            yScale: 1
        }

        states: [
            State {
                name: "globalHover"
                when: myControl.enabled && spinBoxIndicator.enabled && !myControl.drag
                      && !spinBoxIndicator.hover && myControl.hover && !myControl.edit
                PropertyChanges {
                    target: spinBoxIndicatorIcon
                    color: StudioTheme.Values.themeTextColor
                }
            },
            State {
                name: "hover"
                when: myControl.enabled && spinBoxIndicator.enabled && !myControl.drag
                      && spinBoxIndicator.hover && myControl.hover && !spinBoxIndicator.pressed
                PropertyChanges {
                    target: spinBoxIndicatorIcon
                    color: StudioTheme.Values.themeIconColorHover
                }
            },
            State {
                name: "press"
                when: myControl.enabled && spinBoxIndicator.enabled && !myControl.drag
                      && spinBoxIndicator.pressed
                PropertyChanges {
                    target: spinBoxIndicatorIcon
                    color: "#323232" // TODO
                }
            },
            State {
                name: "edit"
                when: myControl.edit
                PropertyChanges {
                    target: spinBoxIndicatorIcon
                    color: StudioTheme.Values.themeTextColor
                }
            },
            State {
                name: "disable"
                when: !myControl.enabled || !spinBoxIndicator.enabled
                PropertyChanges {
                    target: spinBoxIndicatorIcon
                    color: StudioTheme.Values.themeTextColorDisabled
                }
            }
        ]
    }

    states: [
        State {
            name: "default"
            when: myControl.enabled && !myControl.edit
                  && !spinBoxIndicator.hover && !myControl.hover && !myControl.drag
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: false
            }
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeControlBackground
            }
        },
        State {
            name: "globalHover"
            when: myControl.enabled && spinBoxIndicator.enabled && !myControl.drag
                  && !spinBoxIndicator.hover && myControl.hover && !myControl.edit
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: true
            }
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeControlBackgroundGlobalHover
            }
        },
        State {
            name: "hover"
            when: myControl.enabled && !myControl.drag
                  && spinBoxIndicator.hover && myControl.hover && !spinBoxIndicator.pressed
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: true
            }
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeControlBackgroundHover
            }
        },
        State {
            name: "press"
            when: myControl.enabled && spinBoxIndicator.enabled && !myControl.drag
                  && spinBoxIndicator.pressed
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: true
            }
            PropertyChanges {
                target: spinBoxIndicator
                color: "#2aafd3" // TODO
            }
        },
        State {
            name: "edit"
            when: myControl.edit
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: true
            }
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeControlBackground
            }
        },
        State {
            name: "drag"
            when: myControl.drag
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: false
            }
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeControlBackgroundInteraction
            }
        },
        State {
            name: "disable"
            when: !myControl.enabled
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: false
            }
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeControlBackground
            }
        },
        State {
            name: "limit"
            when: !spinBoxIndicator.enabled && !spinBoxIndicator.realEnabled && myControl.hover
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: true
            }
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeControlBackground
            }
        }
    ]
}
