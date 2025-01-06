// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T

import StudioControls as StudioControls
import StudioTheme as StudioTheme

Item {
    id: root

    enum NotificationType {
        Normal,
        Indeterminate,
        Success,
        Error
    }

    property int type: ShareNotification.NotificationType.Normal

    width: parent.width
    height: StudioTheme.Values.height * 2
    visible: false

    function setText(value: string) {
        label.text = value
    }

    function setHelperText(value: string) {
        helperText.text = value
    }

    function resetHelperText() {
        helperText.text = ""
    }

    function setProgress(value: var) {
        helperText.text = `${value.toFixed(0)} %`
        progressBar.value = value / 100.0
    }

    function hasFinished() {
        return root.type === ShareNotification.NotificationType.Success
            || root.type === ShareNotification.NotificationType.Error
    }

    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: 8
        spacing: 4

        RowLayout {
            width: parent.width

            Text {
                id: label
                Layout.fillWidth: true
                color: StudioTheme.Values.themeTextColor
            }

            Item {
                implicitWidth: StudioTheme.Values.controlLabelWidth
                implicitHeight: StudioTheme.Values.controlLabelWidth

                Label {
                    id: statusIcon
                    anchors.fill: parent
                    color: StudioTheme.Values.themeIconColor
                    font.family: StudioTheme.Constants.iconFont.family
                    font.pixelSize: StudioTheme.Values.myIconFontSize
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        T.ProgressBar {
            id: progressBar

            property color color

            width: parent.width
            height: 8

            value: 0.5

            background: Rectangle {
                id: track
                implicitWidth: 200
                implicitHeight: 6
                color: StudioTheme.Values.themeScrollBarTrack
            }

            contentItem: Item {
                implicitWidth: 200
                implicitHeight: 6
                clip: true

                // Progress indicator for determinate state.
                Rectangle {
                    id: barIndicator
                    width: progressBar.visualPosition * parent.width
                    height: parent.height
                    color: progressBar.color
                    visible: !progressBar.indeterminate
                }

                // Scrolling animation for indeterminate state.
                Rectangle {
                    id: barIndicatorIndeterminate
                    width: parent.width * 0.5
                    height: parent.height
                    color: progressBar.color
                    visible: progressBar.indeterminate

                    XAnimator on x {
                        duration: 650
                        from: -barIndicatorIndeterminate.width
                        to: progressBar.width
                        loops: Animation.Infinite
                        running: progressBar.indeterminate
                    }
                }
            }
        }

        Text {
            id: helperText
            color: StudioTheme.Values.themeTextColor
        }
    }

    states: [
        State {
            name: "normal"
            when: root.type === ShareNotification.NotificationType.Normal

            PropertyChanges {
                target: progressBar
                color: StudioTheme.Values.themeInteraction
                indeterminate: false
            }
            PropertyChanges {
                target: statusIcon
                visible: false
            }
        },
        State {
            name: "error"
            when: root.type === ShareNotification.NotificationType.Error

            PropertyChanges {
                target: progressBar
                color: StudioTheme.Values.themeRedLight
                indeterminate: false
                value: 1
            }
            PropertyChanges {
                target: statusIcon
                visible: true
                color: StudioTheme.Values.themeRedLight
                text: StudioTheme.Constants.error_medium
            }
        },
        State {
            name: "success"
            when: root.type === ShareNotification.NotificationType.Success

            PropertyChanges {
                target: progressBar
                color: StudioTheme.Values.themeGreenLight
                indeterminate: false
                value: 1
            }
            PropertyChanges {
                target: statusIcon
                visible: true
                color: StudioTheme.Values.themeGreenLight
                text: StudioTheme.Constants.apply_medium
            }
        },
        State {
            name: "indeterminate"
            when: root.type === ShareNotification.NotificationType.Indeterminate

            PropertyChanges {
                target: progressBar
                color: StudioTheme.Values.themeInteraction
                indeterminate: true
            }
            PropertyChanges {
                target: statusIcon
                visible: false
            }
        }
    ]
}
