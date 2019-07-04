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
import QtQuick.Shapes 1.12
import QtQuick.Templates 2.12 as T
import StudioTheme 1.0 as StudioTheme

T.Slider {
    id: slider

    property int decimals: 0
    property bool labels: true
    property bool tickMarks: false
    property real tickMarkStepSize: 0.0 // StepSize bug QTBUG-76136
    property real tickMarkWidth: 1.0
    property real tickMarkHeight: 4.0
    readonly property int tickMarkCount: tickMarkStepSize
                                         !== 0.0 ? (to - from) / tickMarkStepSize + 1 : 0
    readonly property real tickMarkSpacing: tickMarkCount
                                            !== 0 ? (sliderTrack.width - tickMarkWidth
                                                     * tickMarkCount) / (tickMarkCount - 1) : 0.0

    property string __activeColor: StudioTheme.Values.themeSliderActiveTrack
    property string __inactiveColor: StudioTheme.Values.themeSliderInactiveTrack

    property bool hover: false // This property is used to indicate the global hover state
    property bool edit: slider.activeFocus

    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: StudioTheme.Values.squareComponentWidth
    property real __actionIndicatorHeight: StudioTheme.Values.height

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitHandleWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitHandleHeight + topPadding + bottomPadding,
                             StudioTheme.Values.height)
    padding: 0
    leftPadding: actionIndicator.width
                 - (actionIndicatorVisible ? StudioTheme.Values.border
                                             - StudioTheme.Values.sliderPadding : 0)

    wheelEnabled: false

    ActionIndicator {
        id: actionIndicator
        myControl: slider
        x: 0
        y: 0
        width: actionIndicator.visible ? __actionIndicatorWidth : 0
        height: actionIndicator.visible ? __actionIndicatorHeight : 0
    }

    handle: Rectangle {
        id: sliderHandle
        x: slider.leftPadding + (slider.visualPosition * slider.availableWidth)
           - (sliderHandle.width / 2)
        y: slider.topPadding + (slider.availableHeight / 2) - (sliderHandle.height / 2)
        z: 20
        implicitWidth: StudioTheme.Values.sliderHandleWidth
        implicitHeight: StudioTheme.Values.sliderHandleHeight
        color: StudioTheme.Values.themeSliderHandle

        Shape {
            id: sliderHandleLabelPointer

            property real __width: StudioTheme.Values.sliderPointerWidth
            property real __height: StudioTheme.Values.sliderPointerHeight
            property bool antiAlias: true

            layer.enabled: antiAlias
            layer.smooth: antiAlias
            layer.textureSize: Qt.size(width * 2, height * 2)

            implicitWidth: __width
            implicitHeight: __height

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: sliderHandleLabelBackground.bottom

            ShapePath {
                id: sliderHandleLabelPointerPath
                strokeColor: "transparent"
                strokeWidth: 0
                fillColor: StudioTheme.Values.themeInteraction

                startX: 0
                startY: 0

                PathLine {
                    x: sliderHandleLabelPointer.__width
                    y: 0
                }
                PathLine {
                    x: sliderHandleLabelPointer.__width / 2
                    y: sliderHandleLabelPointer.__height
                }
            }
        }

        Rectangle {
            id: sliderHandleLabelBackground
            x: -(sliderHandleLabelBackground.width / 2) + (sliderHandle.width / 2)
            width: makeEven(
                       sliderHandleLabel.width + StudioTheme.Values.inputHorizontalPadding)
            height: sliderHandleLabel.height
            anchors.bottom: parent.top
            anchors.bottomMargin: StudioTheme.Values.sliderMargin
            color: StudioTheme.Values.themeInteraction

            Text {
                id: sliderHandleLabel
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                text: Number.parseFloat(slider.value).toFixed(slider.decimals)
                color: StudioTheme.Values.themeTextColor
                font.pixelSize: StudioTheme.Values.sliderFontSize
            }
        }
    }

    function makeEven(value) {
        var v = Math.round(value)
        return (v % 2 === 0) ? v : v + 1
    }

    background: Rectangle {
        id: sliderTrack
        x: slider.leftPadding
        y: slider.topPadding + slider.availableHeight / 2 - height / 2
        width: slider.availableWidth
        height: StudioTheme.Values.sliderTrackHeight
        color: __inactiveColor

        Rectangle {
            width: slider.visualPosition * parent.width
            height: parent.height
            color: __activeColor
        }
    }

    Item {
        id: tickmarkBounds
        x: sliderTrack.x
        y: sliderTrack.y

        Text {
            id: tickmarkFromLabel
            x: 0
            y: StudioTheme.Values.sliderPadding
            text: Number.parseFloat(slider.from).toFixed(slider.decimals)
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.sliderFontSize
            visible: slider.labels
        }

        Text {
            id: tickmarkToLabel
            x: slider.availableWidth - width
            y: StudioTheme.Values.sliderPadding
            text: Number.parseFloat(slider.to).toFixed(slider.decimals)
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.sliderFontSize
            visible: slider.labels
        }

        Row {
            id: tickmarkRow
            spacing: tickMarkSpacing
            visible: slider.tickMarks

            Repeater {
                id: tickmarkRepeater
                model: tickMarkCount
                delegate: Rectangle {
                    implicitWidth: tickMarkWidth
                    implicitHeight: StudioTheme.Values.sliderTrackHeight
                    color: x < (slider.visualPosition
                                * slider.availableWidth) ? __inactiveColor : __activeColor
                }
            }
        }
    }

    MouseArea {
        id: mouseArea
        x: actionIndicator.width
        y: 0
        width: slider.width - actionIndicator.width
        height: slider.height
        enabled: true
        hoverEnabled: true
        propagateComposedEvents: true
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.PointingHandCursor
        // Sets the global hover
        onContainsMouseChanged: slider.hover = containsMouse
        onPressed: mouse.accepted = false
    }

    states: [
        State {
            name: "default"
            when: slider.enabled && !slider.hover && !slider.edit
            PropertyChanges {
                target: slider
                wheelEnabled: false
            }
        },
        State {
            name: "hovered"
            when: slider.enabled && slider.hover && !slider.edit
            PropertyChanges {
                target: slider
                __activeColor: StudioTheme.Values.themeSliderActiveTrackHover
                __inactiveColor: StudioTheme.Values.themeSliderInactiveTrackHover
            }
            PropertyChanges {
                target: sliderHandle
                color: StudioTheme.Values.themeSliderHandleHover
            }
        },
        State {
            name: "focus"
            when: slider.enabled && slider.edit
            PropertyChanges {
                target: slider
                wheelEnabled: true
                __activeColor: StudioTheme.Values.themeSliderActiveTrackFocus
                __inactiveColor: StudioTheme.Values.themeSliderInactiveTrackFocus
            }
            PropertyChanges {
                target: sliderHandle
                color: StudioTheme.Values.themeSliderHandleFocus
            }
        },
        State {
            name: "disabled"
            when: !slider.enabled
            PropertyChanges {
                target: tickmarkFromLabel
                color: StudioTheme.Values.themeTextColorDisabled
            }
            PropertyChanges {
                target: tickmarkToLabel
                color: StudioTheme.Values.themeTextColorDisabled
            }
            PropertyChanges {
                target: sliderHandleLabel
                color: StudioTheme.Values.themeTextColorDisabled
            }
            PropertyChanges {
                target: slider
                __activeColor: StudioTheme.Values.themeControlBackgroundDisabled
                __inactiveColor: StudioTheme.Values.themeControlBackgroundDisabled
            }
            PropertyChanges {
                target: sliderHandleLabelBackground
                color: StudioTheme.Values.themeControlBackgroundDisabled
            }
            PropertyChanges {
                target: sliderHandleLabelPointerPath
                fillColor: StudioTheme.Values.themeControlBackgroundDisabled
            }
            PropertyChanges {
                target: sliderHandle
                color: StudioTheme.Values.themeControlBackgroundDisabled
            }
        }
    ]
}
