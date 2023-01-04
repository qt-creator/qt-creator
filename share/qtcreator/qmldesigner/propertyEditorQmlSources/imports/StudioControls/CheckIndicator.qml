// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    property bool hasActiveDrag: myControl.hasActiveDrag ?? false
    property bool hasActiveHoverDrag: myControl.hasActiveHoverDrag ?? false

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
        onClicked: {
            if (myControl.activeFocus)
                myControl.focus = false

            if (myPopup.opened) {
                myPopup.close()
            } else {
                myPopup.open()
                myPopup.forceActiveFocus()
            }
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
                  && !checkIndicator.checked && !checkIndicator.hasActiveDrag
            PropertyChanges {
                target: checkIndicator
                color: StudioTheme.Values.themeControlBackground
            }
        },
        State {
            name: "dragHover"
            when: myControl.enabled && checkIndicator.hasActiveHoverDrag
            PropertyChanges {
                target: checkIndicator
                color: StudioTheme.Values.themeControlBackgroundInteraction
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
                color: StudioTheme.Values.themeIconColor
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
                color: StudioTheme.Values.themeIconColor
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
