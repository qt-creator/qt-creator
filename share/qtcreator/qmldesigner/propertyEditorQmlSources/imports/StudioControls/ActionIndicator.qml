// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: actionIndicator

    property Item myControl

    property alias icon: actionIndicatorIcon

    property bool hover: false
    property bool pressed: false
    property bool forceVisible: false

    color: "transparent"

    implicitWidth: StudioTheme.Values.actionIndicatorWidth
    implicitHeight: StudioTheme.Values.actionIndicatorHeight

    signal clicked
    z: 10

    T.Label {
        id: actionIndicatorIcon
        anchors.fill: parent
        text: StudioTheme.Constants.actionIcon
        visible: text !== StudioTheme.Constants.actionIcon || actionIndicator.forceVisible
                 || (myControl !== undefined &&
                     ((myControl.edit !== undefined && myControl.edit)
                      || (myControl.hover !== undefined && myControl.hover)
                      || (myControl.drag !== undefined && myControl.drag)))
        color: StudioTheme.Values.themeTextColor
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: StudioTheme.Values.myIconFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        states: [
            State {
                name: "hover"
                when: actionIndicator.hover && !actionIndicator.pressed
                      && (!myControl || (!myControl.edit && !myControl.drag))
                      && actionIndicator.enabled
                PropertyChanges {
                    target: actionIndicatorIcon
                    scale: 1.2
                    visible: true
                }
            },
            State {
                name: "disable"
                when: !actionIndicator.enabled
                PropertyChanges {
                    target: actionIndicatorIcon
                    color: StudioTheme.Values.themeTextColorDisabled
                }
            }
        ]
    }

    MouseArea {
        id: actionIndicatorMouseArea
        anchors.fill: parent
        hoverEnabled: true
        onContainsMouseChanged: actionIndicator.hover = containsMouse
        onClicked: actionIndicator.clicked()
    }
}
