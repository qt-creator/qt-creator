// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Rectangle {
    id: root

    property int toolTipDelay: 1000
    property bool qdsTrusted: isQDSTrusted()

    width: 260
    height: root.qdsTrusted ? 150 : 210
    color: StudioTheme.Values.themePanelBackground
    border.color: StudioTheme.Values.themeControlOutline
    border.width: StudioTheme.Values.border

    function handleSpeedChanged() {
        speedSlider.value = Math.round(speed)
    }

    function handleMultiplierChanged() {
        multiplierSpin.value = multiplier
    }

    // Connect context object signals to our handler functions
    // Controls lose the initial binding if the value changes so we need these handlers
    Component.onCompleted: {
        onSpeedChanged.connect(handleSpeedChanged);
        onMultiplierChanged.connect(handleMultiplierChanged);
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            height: 32
            Layout.topMargin: 8
            Layout.rightMargin: 8
            Layout.leftMargin: 8
            Layout.fillWidth: true
            spacing: 16

            Rectangle {
                width: 40
                height: 40
                radius: 5
                Layout.fillHeight: false
                color: StudioTheme.Values.themePanelBackground
                border.color: StudioTheme.Values.themeControlOutline
                border.width: StudioTheme.Values.border

                HelperWidgets.IconIndicator {
                    anchors.fill: parent
                    icon: StudioTheme.Constants.cameraSpeed_medium
                    pixelSize: StudioTheme.Values.myIconFontSize * 1.4
                    iconColor: StudioTheme.Values.themeLinkIndicatorColorHover
                    enabled: false
                    states: [] // Disable normal state based coloring
                }
            }
            Text {
                text: qsTr("Camera Speed Configuration")
                font.pixelSize: 12
                horizontalAlignment: Text.AlignLeft
                Layout.fillWidth: true
                font.bold: true
                color: StudioTheme.Values.themeTextColor
            }
        }

        ColumnLayout {
            Layout.margins: 10
            Layout.fillWidth: true
            Layout.fillHeight: true

            RowLayout {
                Layout.fillWidth: true
                spacing: 5

                StudioControls.Slider {
                    id: speedSlider
                    Layout.fillWidth: true
                    labels: false
                    actionIndicatorVisible: false
                    handleLabelVisible: false
                    from: 1
                    to: 100
                    value: Math.round(speed)
                    onMoved: speed = Math.round(value)

                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The speed camera moves when controlled by keyboard.")
                    ToolTip.delay: root.toolTipDelay
                }

                Text {
                    Layout.preferredWidth: 80
                    text: {
                        const decimals = -Math.floor(Math.log10(multiplier))
                        return totalSpeed.toLocaleString(Qt.locale(), 'f', decimals > 0 ? decimals : 0)
                    }

                    font.pixelSize: 12
                    font.bold: true
                    horizontalAlignment: Qt.AlignRight
                    color: StudioTheme.Values.themeTextColor
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 5

                Text {
                    text: qsTr("Multiplier")
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignLeft
                    Layout.fillWidth: true
                    font.bold: true
                    color: StudioTheme.Values.themeTextColor
                }

                HelperWidgets.DoubleSpinBox {
                    id: multiplierSpin
                    Layout.fillWidth: true
                    minimumValue: 0.01
                    maximumValue: 100000
                    value: multiplier
                    stepSize: 0.01
                    decimals: 2

                    ToolTip.visible: hover
                    ToolTip.text: qsTr("The value multiplier for the speed slider.")
                    ToolTip.delay: root.toolTipDelay

                    onValueChanged: multiplier = value
                }

                HelperWidgets.Button {
                    text: qsTr("Reset")
                    Layout.alignment: Qt.AlignBottom
                    Layout.preferredWidth: 70
                    Layout.preferredHeight: multiplierSpin.height
                    onClicked: resetDefaults()
                }
            }

            Rectangle {
                visible: !root.qdsTrusted
                color: "transparent"
                border.color: StudioTheme.Values.themeWarning
                Layout.fillWidth: true
                Layout.fillHeight: true

                RowLayout {
                    anchors.fill: parent

                    HelperWidgets.IconLabel {
                        icon: StudioTheme.Constants.warning_medium
                        pixelSize: StudioTheme.Values.mediumIconFontSize
                        Layout.leftMargin: 10
                    }

                    Text {
                        text: qsTr('<p>You only have partial control in fly mode. For full control, please
                               enable the <span style="text-decoration: underline">Accessibility settings</span></p>')

                        color: StudioTheme.Values.themeTextColor
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        Layout.margins: 6
                        textFormat: Text.RichText

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                Qt.openUrlExternally("x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility")
                                accessibilityOpened()
                            }
                        }
                    }
                }
            }
        }
    }
}
