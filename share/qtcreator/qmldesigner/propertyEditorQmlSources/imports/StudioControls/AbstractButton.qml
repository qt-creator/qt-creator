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

T.AbstractButton {
    id: myButton

    property alias buttonIcon: buttonIcon.text
    property alias iconColor: buttonIcon.color
    property alias iconFont: buttonIcon.font.family
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

    onHoveredChanged: {
        if (parent !== undefined && parent.hover !== undefined)
            parent.hover = hovered
    }

    background: Rectangle {
        id: buttonBackground
        color: myButton.checked ? StudioTheme.Values.themeControlBackgroundChecked : StudioTheme.Values.themeControlBackground
        border.color: myButton.checked ? StudioTheme.Values.themeInteraction : StudioTheme.Values.themeControlOutline
        border.width: StudioTheme.Values.border
    }

    indicator: Item {
        x: 0
        y: 0
        implicitWidth: myButton.width
        implicitHeight: myButton.height

        T.Label {
            id: buttonIcon
            color: StudioTheme.Values.themeTextColor
            font.family: StudioTheme.Constants.iconFont.family
            font.pixelSize: StudioTheme.Values.myIconFontSize
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            anchors.fill: parent
            renderType: Text.QtRendering
        }
    }

    states: [
        State {
            name: "default"
            when: myButton.enabled && !myButton.hovered && !myButton.pressed
                  && !myButton.checked
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
            name: "hovered"
            when: myButton.hovered && !myButton.pressed
            PropertyChanges {
                target: buttonBackground
                color: StudioTheme.Values.themeHoverHighlight
            }
        },
        State {
            name: "pressed"
            when: myButton.hovered && myButton.pressed
            PropertyChanges {
                target: buttonBackground
                color: StudioTheme.Values.themeControlBackgroundPressed
                border.color: StudioTheme.Values.themeInteraction
            }
            PropertyChanges {
                target: myButton
                z: 10
            }
        },
        State {
            name: "disabled"
            when: !myButton.enabled
            PropertyChanges {
                target: buttonBackground
                color: StudioTheme.Values.themeControlBackgroundDisabled
                border.color: StudioTheme.Values.themeControlOutlineDisabled
            }
            PropertyChanges {
                target: buttonIcon
                color: StudioTheme.Values.themeTextColorDisabled
            }
        }
    ]
}
