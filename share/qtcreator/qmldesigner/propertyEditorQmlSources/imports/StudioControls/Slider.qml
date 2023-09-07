// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Shapes
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.Slider {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property int decimals: 0
    property bool labels: true
    property bool tickMarks: false
    property real tickMarkStepSize: 0.0 // StepSize bug QTBUG-76136
    property real tickMarkWidth: 1.0
    property real tickMarkHeight: 4.0
    readonly property int tickMarkCount: control.tickMarkStepSize !== 0.0
                                         ? (control.to - control.from) / control.tickMarkStepSize + 1 : 0
    readonly property real tickMarkSpacing: control.tickMarkCount !== 0
                                            ? (sliderTrack.width - control.tickMarkWidth
                                               * control.tickMarkCount) / (control.tickMarkCount - 1) : 0.0

    property string __activeColor: control.style.slider.activeTrack
    property string __inactiveColor: control.style.slider.inactiveTrack

    property bool hover: false // This property is used to indicate the global hover state
    property bool edit: control.activeFocus
    property bool handleLabelVisible: true

    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: control.style.actionIndicatorSize.width
    property real __actionIndicatorHeight: control.style.actionIndicatorSize.height

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitHandleWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitHandleHeight + topPadding + bottomPadding,
                             control.style.controlSize.height)
    padding: 0
    leftPadding: actionIndicator.width
                 - (control.actionIndicatorVisible ? control.style.borderWidth
                                             - control.style.sliderPadding : 0)

    wheelEnabled: false

    ActionIndicator {
        id: actionIndicator
        style: control.style
        __parentControl: control
        x: 0
        y: 0
        width: actionIndicator.visible ? control.__actionIndicatorWidth : 0
        height: actionIndicator.visible ? control.__actionIndicatorHeight : 0
    }

    handle: Rectangle {
        id: sliderHandle
        x: control.leftPadding + (control.visualPosition * control.availableWidth)
           - (sliderHandle.width / 2)
        y: control.topPadding + (control.availableHeight / 2) - (sliderHandle.height / 2)
        z: 20
        implicitWidth: control.style.sliderHandleSize.width
        implicitHeight: control.style.sliderHandleSize.height
        color: control.style.slider.handle

        Shape {
            id: sliderHandleLabelPointer

            property real __width: control.style.sliderPointerSize.width
            property real __height: control.style.sliderPointerSize.height
            property bool antiAlias: true

            layer.enabled: sliderHandleLabelPointer.antiAlias
            layer.smooth: sliderHandleLabelPointer.antiAlias
            layer.textureSize: Qt.size(sliderHandleLabelPointer.width * 2,
                                       sliderHandleLabelPointer.height * 2)

            implicitWidth: sliderHandleLabelPointer.__width
            implicitHeight: sliderHandleLabelPointer.__height

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: sliderHandleLabelBackground.bottom

            visible: control.handleLabelVisible

            ShapePath {
                id: sliderHandleLabelPointerPath
                strokeColor: "transparent"
                strokeWidth: 0
                fillColor: control.style.interaction

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
            width: control.makeEven(
                       sliderHandleLabel.width + control.style.inputHorizontalPadding)
            height: sliderHandleLabel.height
            anchors.bottom: parent.top
            anchors.bottomMargin: control.style.sliderMargin
            color: control.style.interaction
            visible: control.handleLabelVisible

            Text {
                id: sliderHandleLabel
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                text: Number.parseFloat(control.value).toFixed(control.decimals)
                color: control.style.text.idle
                font.pixelSize: control.style.smallFontSize
            }
        }
    }

    function makeEven(value) {
        var v = Math.round(value)
        return (v % 2 === 0) ? v : v + 1
    }

    background: Rectangle {
        id: sliderTrack
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - sliderTrack.height / 2
        width: control.availableWidth
        height: control.style.sliderTrackHeight
        color: control.__inactiveColor

        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            color: control.__activeColor
        }
    }

    Item {
        id: tickmarkBounds
        x: sliderTrack.x
        y: sliderTrack.y

        Text {
            id: tickmarkFromLabel
            x: 0
            y: control.style.sliderPadding
            text: Number.parseFloat(control.from).toFixed(control.decimals)
            color: control.style.text.idle
            font.pixelSize: control.style.smallFontSize
            visible: control.labels
        }

        Text {
            id: tickmarkToLabel
            x: control.availableWidth - tickmarkToLabel.width
            y: control.style.sliderPadding
            text: Number.parseFloat(control.to).toFixed(control.decimals)
            color: control.style.text.idle
            font.pixelSize: control.style.smallFontSize
            visible: control.labels
        }

        Row {
            id: tickmarkRow
            spacing: control.tickMarkSpacing
            visible: control.tickMarks

            Repeater {
                id: tickmarkRepeater
                model: control.tickMarkCount
                delegate: Rectangle {
                    implicitWidth: control.tickMarkWidth
                    implicitHeight: control.style.sliderTrackHeight
                    color: x < (control.visualPosition
                                * control.availableWidth) ? control.__inactiveColor
                                                          : control.__activeColor
                }
            }
        }
    }

    MouseArea {
        id: mouseArea
        x: actionIndicator.width
        y: 0
        width: control.width - actionIndicator.width
        height: control.height
        enabled: true
        hoverEnabled: true
        propagateComposedEvents: true
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.PointingHandCursor
        // Sets the global hover
        onContainsMouseChanged: control.hover = mouseArea.containsMouse
        onPressed: function(mouse) { mouse.accepted = false }
    }

    states: [
        State {
            name: "default"
            when: control.enabled && !control.hover && !control.edit
            PropertyChanges {
                target: control
                wheelEnabled: false
            }
        },
        State {
            name: "hover"
            when: control.enabled && control.hover && !control.edit
            PropertyChanges {
                target: control
                __activeColor: control.style.slider.activeTrackHover
                __inactiveColor: control.style.slider.inactiveTrackHover
            }
            PropertyChanges {
                target: sliderHandle
                color: control.style.slider.handleHover
            }
        },
        State {
            name: "focus"
            when: control.enabled && control.edit
            PropertyChanges {
                target: control
                wheelEnabled: true
                __activeColor: control.style.slider.activeTrackFocus
                __inactiveColor: control.style.slider.inactiveTrackFocus
            }
            PropertyChanges {
                target: sliderHandle
                color: control.style.slider.handleFocus
            }
        },
        State {
            name: "disable"
            when: !control.enabled
            PropertyChanges {
                target: tickmarkFromLabel
                color: control.style.text.disabled
            }
            PropertyChanges {
                target: tickmarkToLabel
                color: control.style.text.disabled
            }
            PropertyChanges {
                target: sliderHandleLabel
                color: control.style.text.disabled
            }
            PropertyChanges {
                target: control
                __activeColor: control.style.background.disabled
                __inactiveColor: control.style.background.disabled
            }
            PropertyChanges {
                target: sliderHandleLabelBackground
                color: control.style.background.disabled
            }
            PropertyChanges {
                target: sliderHandleLabelPointerPath
                fillColor: control.style.background.disabled
            }
            PropertyChanges {
                target: sliderHandle
                color: control.style.background.disabled
            }
        }
    ]
}
