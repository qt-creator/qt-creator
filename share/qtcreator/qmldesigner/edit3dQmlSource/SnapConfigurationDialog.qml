// Copyright (C) 2023 The Qt Company Ltd.
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

    width: 230
    height: 270
    color: StudioTheme.Values.themePanelBackground
    border.color: StudioTheme.Values.themeControlOutline
    border.width: StudioTheme.Values.border

    function handlePosIntChanged() {
        posIntSpin.value = posInt
    }

    function handleRotIntChanged() {
        rotIntSpin.value = rotInt
    }

    function handleScaleIntChanged() {
        scaleIntSpin.value = scaleInt
    }

    // Connect context object signals to our handler functions
    // Spinboxes lose the initial binding if the value changes so we need these handlers
    Component.onCompleted: {
        onPosIntChanged.connect(handlePosIntChanged);
        onRotIntChanged.connect(handleRotIntChanged);
        onScaleIntChanged.connect(handleScaleIntChanged);
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
                    icon: StudioTheme.Constants.snapping_conf_medium
                    pixelSize: StudioTheme.Values.myIconFontSize * 1.4
                    iconColor: StudioTheme.Values.themeLinkIndicatorColorHover
                    enabled: false
                    states: [] // Disable normal state based coloring
                }
            }
            Text {
                text: qsTr("Snap Configuration")
                font.pixelSize: 12
                horizontalAlignment: Text.AlignLeft
                Layout.fillWidth: true
                font.bold: true
                color: StudioTheme.Values.themeTextColor
            }
        }

        GridLayout {
            Layout.margins:10
            Layout.fillWidth: true
            Layout.fillHeight: true

            rowSpacing: 5
            columnSpacing: 5
            rows: 5
            columns: 3

            Text {
                text: qsTr("Interval")
                Layout.column: 1
                Layout.row: 0
                Layout.leftMargin: 10
                font.pixelSize: 12
                font.bold: true
                color: StudioTheme.Values.themeTextColor
            }

            StudioControls.CheckBox {
                text: qsTr("Position")
                Layout.column: 0
                Layout.row: 1
                Layout.minimumWidth: 100
                checked: posEnabled
                actionIndicatorVisible: false

                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Snap position.")
                ToolTip.delay: root.toolTipDelay

                onToggled: posEnabled = checked
            }

            HelperWidgets.DoubleSpinBox {
                id: posIntSpin
                Layout.fillWidth: true
                Layout.column: 1
                Layout.row: 1
                Layout.leftMargin: 10
                minimumValue: 1
                maximumValue: 10000
                value: posInt
                stepSize: 1
                decimals: 0

                ToolTip.visible: hover
                ToolTip.text: qsTr("Snap interval for move gizmo.")
                ToolTip.delay: root.toolTipDelay

                onValueChanged: posInt = value
            }

            StudioControls.CheckBox {
                text: qsTr("Rotation")
                Layout.column: 0
                Layout.row: 2
                Layout.minimumWidth: 100
                checked: rotEnabled
                actionIndicatorVisible: false

                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Snap rotation.")
                ToolTip.delay: root.toolTipDelay

                onToggled: rotEnabled = checked
            }

            HelperWidgets.DoubleSpinBox {
                id: rotIntSpin
                Layout.fillWidth: true
                Layout.column: 1
                Layout.row: 2
                Layout.leftMargin: 10
                minimumValue: 1
                maximumValue: 90
                value: rotInt
                stepSize: 1
                decimals: 0

                ToolTip.visible: hover
                ToolTip.text: qsTr("Snap interval in degrees for rotation gizmo.")
                ToolTip.delay: root.toolTipDelay

                onValueChanged: rotInt = value
            }

            StudioControls.CheckBox {
                text: qsTr("Scale")
                Layout.column: 0
                Layout.row: 3
                Layout.minimumWidth: 100
                checked: scaleEnabled
                actionIndicatorVisible: false

                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Snap scale.")
                ToolTip.delay: root.toolTipDelay

                onToggled: scaleEnabled = checked
            }

            HelperWidgets.DoubleSpinBox {
                id: scaleIntSpin
                Layout.fillWidth: true
                Layout.column: 1
                Layout.row: 3
                Layout.leftMargin: 10
                minimumValue: 1
                maximumValue: 100
                value: scaleInt
                stepSize: 1
                decimals: 0

                ToolTip.visible: hover
                ToolTip.text: qsTr("Snap interval for scale gizmo in percentage of original scale.")
                ToolTip.delay: root.toolTipDelay

                onValueChanged: scaleInt = value
            }

            StudioControls.CheckBox {
                text: qsTr("Absolute Position")
                Layout.fillWidth: false
                Layout.leftMargin: 0
                Layout.column: 0
                Layout.row: 4
                Layout.columnSpan: 3
                checked: absolute
                actionIndicatorVisible: false

                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Toggles if the position snaps to absolute values or relative to object position.")
                ToolTip.delay: root.toolTipDelay

                onToggled: absolute = checked
            }

            Text {
                text: qsTr("deg")
                font.pixelSize: 12
                Layout.column: 2
                Layout.row: 2
                color: StudioTheme.Values.themeTextColor
            }

            Text {
                text: qsTr("%")
                font.pixelSize: 12
                Layout.column: 2
                Layout.row: 3
                color: StudioTheme.Values.themeTextColor
            }
        }

        HelperWidgets.Button {
            text: qsTr("Reset All")
            Layout.bottomMargin: 8
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            onClicked: resetDefaults()
        }
    }
}
