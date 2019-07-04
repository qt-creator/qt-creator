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

Rectangle {
    id: spinBoxIndicator

    property T.Control myControl

    property bool hover: false
    property bool pressed: false

    property alias iconFlip: spinBoxIndicatorIconScale.yScale

    color: StudioTheme.Values.themeControlBackground
    border.width: 0

    // This MouseArea is a workaround to avoid some hover state related bugs
    // when using the actual signal 'up.hovered'. QTBUG-74688
    MouseArea {
        id: spinBoxIndicatorMouseArea
        anchors.fill: parent
        hoverEnabled: true
        onContainsMouseChanged: spinBoxIndicator.hover = containsMouse
        onPressed: {
            myControl.forceActiveFocus()
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
    }

    states: [
        State {
            name: "default"
            when: myControl.enabled && !(spinBoxIndicator.hover
                                         || myControl.hover)
                  && !spinBoxIndicator.pressed && !myControl.edit
                  && !myControl.drag && spinBoxIndicator.enabled
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeControlBackground
            }
        },
        State {
            name: "hovered"
            when: (spinBoxIndicator.hover || myControl.hover)
                  && !spinBoxIndicator.pressed && !myControl.edit
                  && !myControl.drag && spinBoxIndicator.enabled
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeHoverHighlight
            }
        },
        State {
            name: "pressed"
            when: spinBoxIndicator.pressed && spinBoxIndicator.enabled
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "edit"
            when: myControl.edit && spinBoxIndicator.enabled
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeFocusEdit
            }
        },
        State {
            name: "drag"
            when: myControl.drag && spinBoxIndicator.enabled
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeFocusDrag
            }
        },
        State {
            name: "disabled"
            when: !myControl.enabled || !spinBoxIndicator.enabled
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeControlBackgroundDisabled
            }
            PropertyChanges {
                target: spinBoxIndicatorIcon
                color: StudioTheme.Values.themeTextColorDisabled
            }
        }
    ]
}
