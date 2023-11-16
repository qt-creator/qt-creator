// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property Item __parentControl

    property alias icon: icon

    property bool hover: false
    property bool pressed: false
    property bool forceVisible: false

    color: "transparent"

    implicitWidth: control.style.actionIndicatorSize.width
    implicitHeight: control.style.actionIndicatorSize.height

    signal clicked
    z: 10

    T.Label {
        id: icon
        anchors.fill: parent
        text: StudioTheme.Constants.actionIcon
        visible: icon.text !== StudioTheme.Constants.actionIcon || control.forceVisible
                 || ((control.__parentControl ?? false) &&
                     ((control.__parentControl.edit ?? false)
                      || (control.__parentControl.hover ?? false)
                      || (control.__parentControl.drag ?? false)))
        color: control.style.icon.idle
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: control.style.baseIconFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        states: [
            State {
                name: "hover"
                when: control.hover && !control.pressed
                      && (!control.__parentControl
                          || (!control.__parentControl.edit && !control.__parentControl.drag))
                      && control.enabled
                PropertyChanges {
                    target: icon
                    scale: 1.2
                    visible: true
                }
            },
            State {
                name: "disable"
                when: !control.enabled
                PropertyChanges {
                    target: icon
                    color: control.style.icon.disabled
                }
            }
        ]
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onContainsMouseChanged: control.hover = mouseArea.containsMouse
        onClicked: control.clicked()
    }
}
