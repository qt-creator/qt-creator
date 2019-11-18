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

T.Popup {
    id: sliderPopup

    property T.Control myControl

    dim: false
    closePolicy: T.Popup.CloseOnPressOutside | T.Popup.CloseOnPressOutsideParent
                 | T.Popup.CloseOnEscape | T.Popup.CloseOnReleaseOutside
                 | T.Popup.CloseOnReleaseOutsideParent

    background: Rectangle {
        color: StudioTheme.Values.themeControlBackground
        border.color: StudioTheme.Values.themeInteraction
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
            color: slider.pressed ? StudioTheme.Values.themeInteraction : StudioTheme.Values.themeControlOutline
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
