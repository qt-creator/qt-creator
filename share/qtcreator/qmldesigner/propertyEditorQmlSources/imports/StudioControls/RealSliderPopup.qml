// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

T.Popup {
    id: sliderPopup

    property T.Control myControl

    property bool drag: slider.pressed

    dim: false
    closePolicy: T.Popup.CloseOnPressOutside | T.Popup.CloseOnPressOutsideParent
                 | T.Popup.CloseOnEscape | T.Popup.CloseOnReleaseOutside
                 | T.Popup.CloseOnReleaseOutsideParent

    background: Rectangle {
        color: StudioTheme.Values.themePopupBackground
        border.width: 0
    }

    contentItem: T.Slider {
        id: slider
        anchors.fill: parent

        bottomPadding: 0
        topPadding: 0
        rightPadding: 3
        leftPadding: 3

        from: myControl.realFrom
        value: myControl.realValue
        to: myControl.realTo

        focusPolicy: Qt.NoFocus

        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + (slider.availableHeight / 2) - (height / 2)
            width: StudioTheme.Values.sliderHandleWidth
            height: StudioTheme.Values.sliderHandleHeight
            radius: 0
            color: slider.pressed ? StudioTheme.Values.themeSliderHandleInteraction
                                  : StudioTheme.Values.themeSliderHandle
        }

        background: Rectangle {
            x: slider.leftPadding
            y: slider.topPadding + (slider.availableHeight / 2) - (height / 2)
            width: slider.availableWidth
            height: StudioTheme.Values.sliderTrackHeight
            radius: 0
            color: StudioTheme.Values.themeSliderInactiveTrack

            Rectangle {
                width: slider.visualPosition * parent.width
                height: parent.height
                color: StudioTheme.Values.themeSliderActiveTrack
                radius: 0
            }
        }

        onMoved: {
            myControl.realValue = slider.value
            myControl.realValueModified()
        }
    }
}
