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
    id: checkIndicator

    property T.Control myControl
    property T.Popup myPopup

    property bool hover: false
    property bool checked: false

    color: StudioTheme.Values.themeControlBackground
    border.color: StudioTheme.Values.themeControlOutline
    state: "default"

    Connections {
        target: myPopup
        onClosed: checkIndicator.checked = false
        onOpened: checkIndicator.checked = true
    }

    MouseArea {
        id: checkIndicatorMouseArea
        anchors.fill: parent
        hoverEnabled: true
        onContainsMouseChanged: checkIndicator.hover = checkIndicatorMouseArea.containsMouse
        onPressed: {
            myControl.forceActiveFocus() // TODO
            myPopup.opened ? myPopup.close() : myPopup.open()
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
            when: myControl.enabled && !(checkIndicator.hover
                                         || myControl.hover)
                  && !checkIndicator.checked && !myControl.edit
                  && !myControl.drag
            PropertyChanges {
                target: checkIndicator
                color: StudioTheme.Values.themeControlBackground
                border.color: StudioTheme.Values.themeControlOutline
            }
        },
        State {
            name: "hovered"
            when: (checkIndicator.hover || myControl.hover)
                  && !checkIndicator.checked && !myControl.edit
                  && !myControl.drag
            PropertyChanges {
                target: checkIndicator
                color: StudioTheme.Values.themeHoverHighlight
                border.color: StudioTheme.Values.themeControlOutline
            }
        },
        State {
            name: "checked"
            when: checkIndicator.checked
            PropertyChanges {
                target: checkIndicator
                color: StudioTheme.Values.themeInteraction
                border.color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "edit"
            when: myControl.edit && !checkIndicator.checked
                  && !(checkIndicator.hover && myControl.hover)
            PropertyChanges {
                target: checkIndicator
                color: StudioTheme.Values.themeFocusEdit
                border.color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "drag"
            when: myControl.drag && !checkIndicator.checked
                  && !(checkIndicator.hover && myControl.hover)
            PropertyChanges {
                target: checkIndicator
                color: StudioTheme.Values.themeFocusDrag
                border.color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "disabled"
            when: !myControl.enabled
            PropertyChanges {
                target: checkIndicator
                color: StudioTheme.Values.themeControlBackgroundDisabled
                border.color: StudioTheme.Values.themeControlOutlineDisabled
            }
            PropertyChanges {
                target: checkIndicatorIcon
                color: StudioTheme.Values.themeTextColorDisabled
            }
        }
    ]
}
