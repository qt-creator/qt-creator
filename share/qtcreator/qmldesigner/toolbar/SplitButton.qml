// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Shapes
import QtQuick.Layouts
import QtQuick.Templates as T

import StudioTheme as StudioTheme
import HelperWidgets
import ToolBar

Item {
    id: root

    property StudioTheme.ControlStyle style: StudioTheme.Values.toolbarButtonStyle

    property alias tooltip: toolTipArea.tooltip
    property bool hover: primaryButton.hover || menuButton.hover

    signal clicked()
    signal cancelClicked()
    signal runTargetSelected(targetId: string)
    signal openRunTargets()

    property int runTarget: 0 // index
    property string runTargetName
    property int runManagerState: RunManager.NotRunning
    property int runManagerProgress: 0
    property string runManagerError

    property int menuWidth: Math.max(160, root.width)

    width: root.style.squareControlSize.width
    height: root.style.squareControlSize.height

    function isRunTargetEnabled(index: int): bool {
        let modelIndex = runManagerModel.index(index, 0)
        return runManagerModel.data(modelIndex, RunManagerModel.Enabled)
    }

    function getRunTargetName(index: int): string {
        let modelIndex = runManagerModel.index(index, 0)
        return runManagerModel.data(modelIndex, "targetName")
    }

    ToolTipArea {
        id: toolTipArea
        anchors.fill: parent
        // Without setting the acceptedButtons property the clicked event won't
        // reach the AbstractButton, it will be consumed by the ToolTipArea
        acceptedButtons: Qt.NoButton
    }

    onRunTargetChanged: {
        root.runTargetName = root.getRunTargetName(root.runTarget)
        primaryButton.enabled = root.isRunTargetEnabled(root.runTarget)
    }

    component ProgressCircle: Shape {
        id: shape

        property real from: 0
        property real to: 100
        property bool indeterminate: false
        property real value: 0.5

        property int strokeWidth: 4

        property int radiusX: (shape.width - shape.strokeWidth) / 2
        property int radiusY: (shape.height - shape.strokeWidth) / 2

        width: 20
        height: 20
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            id: background
            strokeColor: "gray"
            strokeWidth: shape.strokeWidth
            fillColor: "transparent"
            capStyle: ShapePath.FlatCap

            PathAngleArc {
                radiusX: shape.radiusX
                radiusY: shape.radiusY
                centerX: shape.width / 2
                centerY: shape.height / 2
                startAngle: 0
                sweepAngle: 360
            }
        }

        ShapePath {
            id: foreground
            strokeColor: StudioTheme.Values.themeInteraction
            strokeWidth: shape.strokeWidth
            fillColor: "transparent"
            capStyle: ShapePath.FlatCap

            PathAngleArc {
                radiusX: shape.radiusX
                radiusY: shape.radiusY
                centerX: shape.width / 2
                centerY: shape.height / 2
                startAngle: -90
                sweepAngle: shape.indeterminate ? 90 : 360 * shape.value
            }
        }
        // Indeterminate rotation animation
        RotationAnimation on rotation {
            loops: Animation.Infinite
            from: 0
            to: 360
            running: shape.indeterminate
            duration: 2000

            onStopped: shape.rotation = 0
        }
    }

    Connections {
        target: runManagerModel

        function onModelReset() {
            root.runTargetName = root.getRunTargetName(root.runTarget)
            primaryButton.enabled = root.isRunTargetEnabled(root.runTarget)
        }
    }

    readonly property bool showProgress: root.runManagerState === RunManager.Packing
                                         || root.runManagerState === RunManager.Sending
                                         || root.runManagerState === RunManager.Starting

    Row {
        T.AbstractButton {
            id: primaryButton

            property StudioTheme.ControlStyle style: root.style

            property bool hover: primaryButton.hovered
            property bool press: primaryButton.pressed

            property alias backgroundVisible: buttonBackground.visible
            property alias backgroundRadius: buttonBackground.radius

            implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                                    implicitContentWidth + leftPadding + rightPadding)
            implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                                     implicitContentHeight + topPadding + bottomPadding)
            width: root.width - menuButton.width// TODO
            height: primaryButton.style.squareControlSize.height
            z: primaryButton.checked ? 10 : 3
            activeFocusOnTab: false

            onClicked: {
                window.close()
                root.clicked()
            }

            background: Rectangle {
                id: buttonBackground
                color: primaryButton.style.background.idle
                border.color: primaryButton.style.border.idle
                border.width: 0//primaryButton.style.borderWidth
                radius: primaryButton.style.radius
                topRightRadius: 0
                bottomRightRadius: 0
            }

            indicator: Item {
                x: 0
                y: 0
                width: primaryButton.width
                height: primaryButton.height

                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    Item {
                        width: 20
                        height: primaryButton.height

                        ProgressCircle {
                            anchors.centerIn: parent
                            visible: root.showProgress
                            indeterminate: root.runManagerState === RunManager.Packing
                                           || root.runManagerState === RunManager.Starting
                            value: root.runManagerProgress / 100
                        }

                        T.Label {
                            visible: !root.showProgress
                            height: primaryButton.height

                            font.family: StudioTheme.Constants.iconFont.family
                            font.pixelSize: primaryButton.style.baseIconFontSize
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            color: {
                                if (root.runManagerState === RunManager.NotRunning)
                                    return primaryButton.press ? primaryButton.style.text.idle
                                                               : primaryButton.hover ? "#2eff68" // running green
                                                                                     : "#649a5d" // idle green
                                else
                                    return primaryButton.press ? primaryButton.style.text.idle
                                                               : primaryButton.hover ? "#cc3c34" // recording red
                                                                                     : "#6a4242" // idle red
                            }

                            text: {
                                if (root.runManagerState === RunManager.NotRunning)
                                    return StudioTheme.Constants.playOutline_medium
                                else
                                    return StudioTheme.Constants.stop_medium
                            }
                        }
                    }

                    T.Label {
                        Layout.fillWidth: true
                        height: primaryButton.height
                        color: primaryButton.enabled ? primaryButton.style.text.idle
                                                     : primaryButton.style.text.disabled
                        font.pixelSize: primaryButton.style.baseFontSize
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignLeft
                        elide: Text.ElideMiddle
                        text: {
                            if (root.runManagerState === RunManager.Packing)
                                return qsTr("Packing")
                            else if (root.runManagerState === RunManager.Sending)
                                return qsTr("Sending")
                            else if (root.runManagerState === RunManager.Starting)
                                return qsTr("Starting")
                            else
                                return root.runTargetName
                        }
                    }

                    T.Label {
                        visible: root.showProgress || root.runManagerError
                        height: primaryButton.height

                        font.family: StudioTheme.Constants.iconFont.family
                        font.pixelSize: StudioTheme.Values.mediumFontSize
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter

                        color: root.runManagerError ? StudioTheme.Values.themeError
                                                    : primaryButton.style.text.idle
                        text: root.runManagerError ? StudioTheme.Constants.error_medium
                                                   : StudioTheme.Constants.close_small

                        ToolTipArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton
                            tooltip: root.runManagerError

                            onClicked: root.cancelClicked()
                        }
                    }
                }
            }

            states: [
                State {
                    // This is only to sync colors with TopLevelComboBox
                    name: "open"
                    when: window.visible
                    PropertyChanges {
                        target: buttonBackground
                        color: primaryButton.style.background.hover
                        border.color: primaryButton.style.border.hover
                    }
                },
                State {
                    name: "default"
                    when: primaryButton.enabled && !root.hover && !primaryButton.hover
                          && !primaryButton.press && !primaryButton.checked
                    PropertyChanges {
                        target: buttonBackground
                        color: primaryButton.style.background.idle
                        border.color: primaryButton.style.border.idle
                    }
                    PropertyChanges {
                        target: primaryButton
                        z: 3
                    }
                },
                State {
                    name: "globalHover"
                    when: root.hover && !primaryButton.hover && !primaryButton.press && primaryButton.enabled
                    PropertyChanges {
                        target: buttonBackground
                        color: primaryButton.style.background.globalHover
                        border.color: primaryButton.style.border.idle
                    }
                },
                State {
                    name: "hover"
                    when: !primaryButton.checked && primaryButton.hover && !primaryButton.press && primaryButton.enabled
                    PropertyChanges {
                        target: buttonBackground
                        color: primaryButton.style.background.hover
                        border.color: primaryButton.style.border.hover
                    }
                    PropertyChanges {
                        target: primaryButton
                        z: 100
                    }
                },
                State {
                    name: "hoverCheck"
                    when: primaryButton.checked && primaryButton.hover && !primaryButton.press && primaryButton.enabled
                    PropertyChanges {
                        target: buttonBackground
                        color: primaryButton.style.background.hover
                        border.color: primaryButton.style.border.hover
                    }
                    PropertyChanges {
                        target: primaryButton
                        z: 100
                    }
                },
                State {
                    name: "press"
                    when: primaryButton.hover && primaryButton.press && primaryButton.enabled
                    PropertyChanges {
                        target: buttonBackground
                        color: primaryButton.style.interaction
                        border.color: primaryButton.style.interaction
                    }
                    PropertyChanges {
                        target: primaryButton
                        z: 100
                    }
                },
                State {
                    name: "check"
                    when: primaryButton.enabled && !primaryButton.press && primaryButton.checked
                    PropertyChanges {
                        target: buttonBackground
                        color: primaryButton.style.background.idle
                        border.color: primaryButton.style.border.idle
                    }
                },
                State {
                    name: "disable"
                    when: !primaryButton.enabled
                    PropertyChanges {
                        target: buttonBackground
                        color: primaryButton.style.background.disabled
                        border.color: primaryButton.style.border.disabled
                    }
                }
            ]
        }

        T.AbstractButton {
            id: menuButton

            property StudioTheme.ControlStyle style: root.style

            property bool hover: menuButton.hovered
            property bool press: menuButton.pressed

            property alias backgroundVisible: menuButtonBackground.visible
            property alias backgroundRadius: menuButtonBackground.radius

            implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                                    implicitContentWidth + leftPadding + rightPadding)
            implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                                     implicitContentHeight + topPadding + bottomPadding)
            width: menuButton.style.squareControlSize.width
            height: menuButton.style.squareControlSize.height
            z: menuButton.checked ? 10 : 3
            activeFocusOnTab: false

            checkable: false
            checked: window.visible
            enabled: root.runManagerState === RunManager.NotRunning

            onClicked: {
                if (window.visible) {
                    window.close()
                } else {
                    var originMapped = root.mapToGlobal(0,0)
                    window.x = originMapped.x
                    window.y = originMapped.y + root.height
                    window.show()
                    window.requestActivate()
                }
            }

            background: Rectangle {
                id: menuButtonBackground
                color: menuButton.style.background.idle
                border.color: menuButton.style.border.idle
                border.width: 0//menuButton.style.borderWidth
                radius: menuButton.style.radius
                topLeftRadius: 0
                bottomLeftRadius: 0
            }

            indicator: T.Label {
                width: menuButton.width
                height: menuButton.height
                color: menuButton.style.icon.idle
                font.family: StudioTheme.Constants.iconFont.family
                font.pixelSize: menuButton.style.baseIconFontSize
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                text: StudioTheme.Constants.upDownSquare2
            }

            states: [
                State {
                    name: "default"
                    when: menuButton.enabled && !root.hover && !menuButton.hover
                          && !menuButton.press && !menuButton.checked
                    PropertyChanges {
                        target: menuButtonBackground
                        color: menuButton.style.background.idle
                        border.color: menuButton.style.border.idle
                    }
                    PropertyChanges {
                        target: menuButton
                        z: 3
                    }
                },
                State {
                    name: "globalHover"
                    when: root.hover && !menuButton.hover && !menuButton.press && menuButton.enabled
                          && !menuButton.checked
                    PropertyChanges {
                        target: menuButtonBackground
                        color: menuButton.style.background.globalHover
                        border.color: menuButton.style.border.idle
                    }
                },
                State {
                    name: "hover"
                    when: !menuButton.checked && menuButton.hover && !menuButton.press && menuButton.enabled
                    PropertyChanges {
                        target: menuButtonBackground
                        color: menuButton.style.background.hover
                        border.color: menuButton.style.border.hover
                    }
                    PropertyChanges {
                        target: menuButton
                        z: 100
                    }
                },
                State {
                    name: "hoverCheck"
                    when: menuButton.checked && menuButton.hover && !menuButton.press && menuButton.enabled
                    PropertyChanges {
                        target: menuButtonBackground
                        color: menuButton.style.interaction
                        border.color: menuButton.style.interaction
                    }
                    PropertyChanges {
                        target: menuButton
                        z: 100
                    }
                },
                State {
                    name: "press"
                    when: menuButton.hover && menuButton.press && menuButton.enabled
                    PropertyChanges {
                        target: menuButtonBackground
                        color: menuButton.style.interaction
                        border.color: menuButton.style.interaction
                    }
                    PropertyChanges {
                        target: menuButton
                        z: 100
                    }
                },
                State {
                    name: "check"
                    when: menuButton.enabled && !menuButton.press && menuButton.checked
                    PropertyChanges {
                        target: menuButtonBackground
                        color: menuButton.style.interaction
                        border.color: menuButton.style.border.idle
                    }
                },
                State {
                    name: "disable"
                    when: !menuButton.enabled
                    PropertyChanges {
                        target: menuButtonBackground
                        color: menuButton.style.background.disabled
                        border.color: menuButton.style.border.disabled
                    }
                }
            ]
        }
    }

    Rectangle {
        id: controlBorder
        anchors.fill: parent
        color: "transparent"
        border.color: menuButton.checked ? StudioTheme.Values.themeInteraction
                                         : root.hover ? menuButton.style.border.hover
                                                      : StudioTheme.Values.controlOutline_toolbarIdle
        border.width: StudioTheme.Values.border
        radius: menuButton.style.radius
    }

    DelegateModel {
        id: visualModel
        model: RunManagerModel { id: runManagerModel }
        delegate: MenuItemDelegate {
            required property string targetName
            required property string targetId
            required property bool targetEnabled
            required property int index

            width: root.menuWidth - 2 * window.padding
            height: root.style.controlSize.height

            myIcon: ""
            myText: targetName
            checked: root.runTarget === index
            enabled: targetEnabled

            onClicked: {
                root.runTargetSelected(targetId)
                window.close()
            }
        }
    }

    Window {
        id: window

        readonly property int padding: 1

        width: root.menuWidth
        height: column.implicitHeight
        visible: false
        flags: Qt.FramelessWindowHint | Qt.Tool | Qt.NoDropShadowWindowHint | Qt.WindowStaysOnTopHint
        modality: Qt.NonModal
        transientParent: null
        color: "transparent"

        onActiveFocusItemChanged: {
            if (window.activeFocusItem === null && !root.hover)
                window.close()
        }

        Rectangle {
            anchors.fill: parent
            color: StudioTheme.Values.themePopupBackground

            Column {
                id: column
                padding: window.padding
                anchors.fill: parent
                spacing: 0

                Repeater { model: visualModel }

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: column.width - 10
                    height: StudioTheme.Values.border
                    color: "#636363" // shadowGrey
                }

                MenuItemDelegate {
                    width: root.menuWidth - 2 * window.padding
                    height: root.style.controlSize.height

                    myText: qsTr("Device Manager")
                    myIcon: StudioTheme.Constants.settings_medium

                    onClicked: {
                        root.openRunTargets()
                        window.close()
                    }
                }
            }
        }
    }
}
