// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.Popup {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property T.Control __parentControl

    property bool drag: slider.pressed

    dim: false
    closePolicy: T.Popup.CloseOnPressOutside | T.Popup.CloseOnPressOutsideParent
                 | T.Popup.CloseOnEscape | T.Popup.CloseOnReleaseOutside
                 | T.Popup.CloseOnReleaseOutsideParent

    background: Rectangle {
        color: control.style.popup.background
        border.width: 0
    }

    contentItem: T.Slider {
        id: slider
        anchors.fill: parent

        bottomPadding: 0
        topPadding: 0
        rightPadding: 3
        leftPadding: 3

        from: control.__parentControl.realFrom
        value: control.__parentControl.realValue
        to: control.__parentControl.realTo

        focusPolicy: Qt.NoFocus

        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + (slider.availableHeight / 2) - (height / 2)
            width: control.style.sliderHandleSize.width
            height: control.style.sliderHandleSize.height
            radius: 0
            color: slider.pressed ? control.style.slider.handleInteraction
                                  : control.style.slider.handle
        }

        background: Rectangle {
            x: slider.leftPadding
            y: slider.topPadding + (slider.availableHeight / 2) - (height / 2)
            width: slider.availableWidth
            height: control.style.sliderTrackHeight
            radius: 0
            color: control.style.slider.inactiveTrack

            Rectangle {
                width: slider.visualPosition * parent.width
                height: parent.height
                color: control.style.slider.activeTrack
                radius: 0
            }
        }

        onMoved: {
            control.__parentControl.realValue = slider.value
            control.__parentControl.realValueModified()
        }
    }
}
