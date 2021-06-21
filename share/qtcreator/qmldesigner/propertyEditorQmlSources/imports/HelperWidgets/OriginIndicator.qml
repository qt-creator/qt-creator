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
import StudioTheme 1.0 as StudioTheme

Item {
    id: root

    property Item myControl

    property real iWidth: (root.width - (2 * StudioTheme.Values.border)) / 3.0
    property real iHeight: (root.height - (2 * StudioTheme.Values.border)) / 3.0

    Rectangle {
        id: indicator
        width: root.iWidth
        height: root.iHeight
        color: StudioTheme.Values.themeInteraction

        states: [
            State {
                name: "default"
                when: myControl.enabled && !myControl.pressed
                PropertyChanges {
                    target: indicator
                    color: StudioTheme.Values.themeInteraction
                }
            },
            State {
                name: "press"
                when: myControl.enabled && myControl.pressed
                PropertyChanges {
                    target: indicator
                    color: StudioTheme.Values.themeIconColorInteraction
                }
            },
            State {
                name: "disable"
                when: !myControl.enabled
                PropertyChanges {
                    target: indicator
                    color: StudioTheme.Values.themeTextColorDisabled
                }
            }
        ]
    }

    states: [
        State {
            name: "center"
            when: root.myControl.origin === "Center"
            PropertyChanges {
                target: indicator
                x: StudioTheme.Values.border + root.iWidth
                y: StudioTheme.Values.border + root.iHeight
            }
        },
        State {
            name: "topLeft"
            when: root.myControl.origin === "TopLeft"
            PropertyChanges {
                target: indicator
                x: StudioTheme.Values.border
                y: StudioTheme.Values.border
            }
        },
        State {
            name: "top"
            when: root.myControl.origin === "Top"
            PropertyChanges {
                target: indicator
                x: StudioTheme.Values.border + root.iWidth
                y: StudioTheme.Values.border
            }
        },
        State {
            name: "topRight"
            when: root.myControl.origin === "TopRight"
            PropertyChanges {
                target: indicator
                x: StudioTheme.Values.border + (2 * root.iWidth)
                y: StudioTheme.Values.border
            }
        },
        State {
            name: "right"
            when: root.myControl.origin === "Right"
            PropertyChanges {
                target: indicator
                x: StudioTheme.Values.border + (2 * root.iWidth)
                y: StudioTheme.Values.border + root.iHeight
            }
        },
        State {
            name: "bottomRight"
            when: root.myControl.origin === "BottomRight"
            PropertyChanges {
                target: indicator
                x: StudioTheme.Values.border + (2 * root.iWidth)
                y: StudioTheme.Values.border + (2 * root.iHeight)
            }
        },
        State {
            name: "bottom"
            when: root.myControl.origin === "Bottom"
            PropertyChanges {
                target: indicator
                x: StudioTheme.Values.border + root.iWidth
                y: StudioTheme.Values.border + (2 * root.iHeight)
            }
        },
        State {
            name: "bottomLeft"
            when: root.myControl.origin === "BottomLeft"
            PropertyChanges {
                target: indicator
                x: StudioTheme.Values.border
                y: StudioTheme.Values.border + (2 * root.iHeight)
            }
        },
        State {
            name: "left"
            when: root.myControl.origin === "Left"
            PropertyChanges {
                target: indicator
                x: StudioTheme.Values.border
                y: StudioTheme.Values.border + root.iHeight
            }
        }
    ]
}
