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
    id: checkIndicator

    property T.Control myControl
    property T.Popup myPopup

    property bool hover: checkIndicatorMouseArea.containsMouse && checkIndicator.enabled
    property bool pressed: checkIndicatorMouseArea.containsPress
    property bool checked: false

    color: StudioTheme.Values.themeControlBackground
    border.width: 0

    Connections {
        target: myPopup
        function onClosed() { checkIndicator.checked = false }
        function onOpened() { checkIndicator.checked = true }
    }

    MouseArea {
        id: checkIndicatorMouseArea
        anchors.fill: parent
        hoverEnabled: true
        onPressed: {
            if (myPopup.opened) {
                myPopup.close()
            } else {
                myPopup.open()
                myPopup.forceActiveFocus()
            }

            if (myControl.activeFocus)
                myControl.focus = false
        }
    }

    T.Label {
        id: checkIndicatorIcon
        anchors.fill: parent
        color: StudioTheme.Values.themeTextColor
        text: StudioTheme.Constants.upDownSquare2
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: StudioTheme.Values.sliderControlSizeMulti
        font.family: StudioTheme.Constants.iconFont.family
    }

    states: [
        State {
            name: "default"
            when: myControl.enabled && checkIndicator.enabled && !myControl.edit
                  && !checkIndicator.hover && !myControl.hover && !myControl.drag
                  && !checkIndicator.checked
            PropertyChanges {
                target: checkIndicator
                color: StudioTheme.Values.themeControlBackground
            }
        },
        State {
            name: "globalHover"
            when: myControl.enabled && checkIndicator.enabled && !myControl.drag
                  && !checkIndicator.hover && myControl.hover && !myControl.edit
                  && !checkIndicator.checked
            PropertyChanges {
                target: checkIndicator
                color: StudioTheme.Values.themeControlBackgroundGlobalHover
            }
        },
        State {
            name: "hover"
            when: myControl.enabled && checkIndicator.enabled && !myControl.drag
                  && checkIndicator.hover && myControl.hover && !checkIndicator.pressed
                  && !checkIndicator.checked
            PropertyChanges {
                target: checkIndicator
                color: StudioTheme.Values.themeControlBackgroundHover
            }
        },
        State {
            name: "check"
            when: checkIndicator.checked
            PropertyChanges {
                target: checkIndicatorIcon
                color: StudioTheme.Values.themeIconColorInteraction
            }
            PropertyChanges {
                target: checkIndicator
                color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "edit"
            when: myControl.edit && !checkIndicator.checked
                  && !(checkIndicator.hover && myControl.hover)
            PropertyChanges {
                target: checkIndicatorIcon
                color: StudioTheme.Values.themeTextColor
            }
            PropertyChanges {
                target: checkIndicator
                color: StudioTheme.Values.themeControlBackground
            }
        },
        State {
            name: "press"
            when: myControl.enabled && checkIndicator.enabled && !myControl.drag
                  && checkIndicator.pressed
            PropertyChanges {
                target: checkIndicatorIcon
                color: StudioTheme.Values.themeIconColorInteraction
            }
            PropertyChanges {
                target: checkIndicator
                color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "drag"
            when: (myControl.drag !== undefined && myControl.drag) && !checkIndicator.checked
                  && !(checkIndicator.hover && myControl.hover)
            PropertyChanges {
                target: checkIndicator
                color: StudioTheme.Values.themeControlBackgroundInteraction
            }
        },
        State {
            name: "disable"
            when: !myControl.enabled
            PropertyChanges {
                target: checkIndicator
                color: StudioTheme.Values.themeControlBackgroundDisabled
            }
            PropertyChanges {
                target: checkIndicatorIcon
                color: StudioTheme.Values.themeTextColorDisabled
            }
        }
    ]
}
