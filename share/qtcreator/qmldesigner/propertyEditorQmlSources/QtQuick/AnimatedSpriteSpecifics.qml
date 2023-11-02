// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        caption: qsTr("Animated Sprite")

        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel {
                text: qsTr("Source")
                tooltip: qsTr("Adds an image from the local file system.")
            }

            SecondColumnLayout {
                UrlChooser {
                    backendValue: backendValues.source
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Frame size")
                tooltip: qsTr("Sets the width and height of the frame.")
                blockedByTemplate: !(backendValues.frameWidth.isAvailable || backendValues.frameHeight.isAvailable)
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.frameWidth
                    minimumValue: 0
                    maximumValue: 8192
                    decimals: 0
                    enabled: backendValue.isAvailable
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel {
                    //: The width of the animated sprite frame
                    text: qsTr("W", "width")
                    tooltip: qsTr("Width.")
                    enabled: backendValues.frameWidth.isAvailable
                }

                Spacer { implicitWidth: StudioTheme.Values.controlGap }

                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.frameHeight
                    minimumValue: 0
                    maximumValue: 8192
                    decimals: 0
                    enabled: backendValue.isAvailable
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel {
                    //: The height of the animated sprite frame
                    text: qsTr("H", "height")
                    tooltip: qsTr("Height.")
                    enabled: backendValues.frameHeight.isAvailable
                }
                /*
                TODO QDS-4836
                Spacer { implicitWidth: StudioTheme.Values.controlGap }

                LinkIndicator2D {}
    */
                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Frame coordinates")
                tooltip: qsTr("Sets the coordinates of the first frame of the animated sprite.")
                blockedByTemplate: !(backendValues.frameX.isAvailable || backendValues.frameY.isAvailable)
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.frameX
                    minimumValue: 0
                    maximumValue: 8192
                    decimals: 0
                    enabled: backendValue.isAvailable
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel {
                    //: The width of the animated sprite frame
                    text: qsTr("X", "Frame X")
                    tooltip: qsTr("Frame X coordinate.")
                    enabled: backendValues.frameX.isAvailable
                }

                Spacer { implicitWidth: StudioTheme.Values.controlGap }

                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.frameY
                    minimumValue: 0
                    maximumValue: 8192
                    decimals: 0
                    enabled: backendValue.isAvailable
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel {
                    //: The height of the animated sprite frame
                    text: qsTr("Y", "Frame Y")
                    tooltip: qsTr("Frame Y coordinate.")
                    enabled: backendValues.frameY.isAvailable
                }
                /*
                TODO QDS-4836
                Spacer { implicitWidth: StudioTheme.Values.controlGap }

                LinkIndicator2D {}
    */
                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Frame count")
                tooltip: qsTr("Sets the number of frames in this animated sprite.")
                blockedByTemplate: !backendValues.frameCount.isAvailable
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.frameCount
                    decimals: 0
                    minimumValue: 0
                    maximumValue: 10000
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            //frame rate OR frame duration OR frame sync should be used
            //frame rate has priority over frame duration
            //frame sync has priority over rate and duration
            PropertyLabel {
                text: qsTr("Frame rate")
                tooltip: qsTr("Sets the number of frames per second to show in the animation.")
                blockedByTemplate: !backendValues.frameRate.isAvailable
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.frameRate
                    decimals: 2
                    minimumValue: 0
                    maximumValue: 1000
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            //frame duration OR frame rate OR frame sync should be used
            //frame rate has priority over frame duration
            //frame sync has priority over rate and duration
            PropertyLabel {
                text: qsTr("Frame duration")
                tooltip: qsTr("Sets the duration of each frame of the animation in milliseconds.")
                blockedByTemplate: !backendValues.frameDuration.isAvailable
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.frameDuration
                    decimals: 0
                    minimumValue: 0
                    maximumValue: 100000
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            //frame sync OR frame rate OR frame duration should be used
            //frame rate has priority over frame duration
            //frame sync has priority over rate and duration
            PropertyLabel {
                text: qsTr("Frame sync")
                tooltip: qsTr("Sets frame advancements one frame each time a frame is rendered to the screen.")
                blockedByTemplate: !backendValues.frameSync.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.frameSync.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.frameSync
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Loops")
                tooltip: qsTr("After playing the animation this many times, the animation will automatically stop.")
                blockedByTemplate: !backendValues.loops.isAvailable
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.loops
                    decimals: 0
                    minimumValue: -1 //AnimatedSprite.Infinite = -1
                    maximumValue: 100000
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Interpolate")
                tooltip: qsTr("If true, interpolation will occur between sprite frames to make the animation appear smoother.")
                blockedByTemplate: !backendValues.interpolate.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.interpolate.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.interpolate
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Finish behavior")
                tooltip: qsTr("Sets the behavior when the animation finishes on its own.")
                blockedByTemplate: !backendValues.finishBehavior.isAvailable
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    scope: "AnimatedSprite"
                    model: ["FinishAtInitialFrame", "FinishAtFinalFrame"]
                    backendValue: backendValues.finishBehavior
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Reverse")
                tooltip: qsTr("If true, the animation will be played in reverse.")
                blockedByTemplate: !backendValues.reverse.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.reverse.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.reverse
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Running")
                tooltip: qsTr("Whether the sprite is animating or not.")
                blockedByTemplate: !backendValues.running.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.running.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.running
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Paused")
                tooltip: qsTr("When paused, the current frame can be advanced manually.")
                blockedByTemplate: !backendValues.paused.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.paused.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.paused
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Current frame")
                tooltip: qsTr("When paused, the current frame can be advanced manually by setting this property.")
                blockedByTemplate: !backendValues.currentFrame.isAvailable
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.currentFrame
                    decimals: 0
                    minimumValue: 0
                    maximumValue: 100000
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }
        }
    }
}
