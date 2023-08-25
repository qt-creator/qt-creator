// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuickDesignerTheme
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Rectangle {
    id: root

    property int toolTipDelay: 1000

    color: StudioTheme.Values.themePanelBackground

    Column {
        id: col
        padding: 8
        spacing: 4

        Rectangle {
            id: ctrlRect
            width: root.width - 16
            height: posIntValue.height + rotIntValue.height + scaleIntValue.height + 32

            color: StudioTheme.Values.themePanelBackground
            border.color: StudioTheme.Values.themeControlOutline
            border.width: StudioTheme.Values.border

            Column {
                padding: 8
                spacing: 8
                Row {
                    height: posIntValue.height
                    width: parent.width - 16
                    spacing: StudioTheme.Values.sectionRowSpacing

                    Text {
                        id: posIntLabel
                        text: qsTr("Position Snap Interval:")
                        color: enabled ? StudioTheme.Values.themeTextColor
                                       : StudioTheme.Values.themeTextColorDisabled
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignRight
                        height: posIntValue.height
                    }

                    Item { // Spacer
                        width: Math.max(ctrlRect.width - posIntLabel.width - posIntValue.width - 32, 1)
                        height: 1
                    }

                    StudioControls.RealSpinBox {
                        id: posIntValue
                        realFrom: 1
                        realTo: 100000
                        realValue: rootView.posInt
                        realStepSize: 1
                        width: 80
                        actionIndicatorVisible: false

                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Snap interval for move gizmo.")
                        ToolTip.delay: root.toolTipDelay

                        onRealValueChanged: rootView.posInt = realValue
                    }
                }

                Row {
                    height: rotIntValue.height
                    width: parent.width - 16
                    spacing: StudioTheme.Values.sectionRowSpacing

                    Text {
                        id: rotIntLabel
                        text: qsTr("Rotation Snap Interval:")
                        color: enabled ? StudioTheme.Values.themeTextColor
                                       : StudioTheme.Values.themeTextColorDisabled
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignRight
                        height: rotIntValue.height
                    }

                    Item { // Spacer
                        width: Math.max(ctrlRect.width - rotIntLabel.width - rotIntValue.width - 32, 1)
                        height: 1
                    }

                    StudioControls.RealSpinBox {
                        id: rotIntValue
                        realFrom: 1
                        realTo: 360
                        realValue: rootView.rotInt
                        realStepSize: 1
                        width: 80
                        actionIndicatorVisible: false

                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Snap interval in degrees for rotation gizmo.")
                        ToolTip.delay: root.toolTipDelay

                        onRealValueChanged: rootView.rotInt = realValue
                    }
                }

                Row {
                    height: scaleIntValue.height
                    width: parent.width - 16
                    spacing: StudioTheme.Values.sectionRowSpacing

                    Text {
                        id: scaleIntLabel
                        text: qsTr("Scale Snap Interval (%):")
                        color: enabled ? StudioTheme.Values.themeTextColor
                                       : StudioTheme.Values.themeTextColorDisabled
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignRight
                        height: scaleIntValue.height
                    }

                    Item { // Spacer
                        width: Math.max(ctrlRect.width - scaleIntLabel.width - scaleIntValue.width - 32, 1)
                        height: 1
                    }

                    StudioControls.RealSpinBox {
                        id: scaleIntValue
                        realFrom: 1
                        realTo: 100000
                        realValue: rootView.scaleInt
                        realStepSize: 1
                        width: 80
                        actionIndicatorVisible: false

                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Snap interval for scale gizmo in percentage of original scale.")
                        ToolTip.delay: root.toolTipDelay

                        onRealValueChanged: rootView.scaleInt = realValue
                    }
                }
            }
        }

        Item { // Spacer
            width: 1
            height: Math.max(root.height - buttons.height - ctrlRect.height - 16, 2)
        }

        Item {
            id: buttons
            height: cancelButton.height + 8
            width: ctrlRect.width

            Row {
                spacing: StudioTheme.Values.dialogButtonSpacing
                height: cancelButton.height
                anchors.right: parent.right

                HelperWidgets.Button {
                    id: cancelButton
                    text: qsTr("Cancel")
                    leftPadding: StudioTheme.Values.dialogButtonPadding
                    rightPadding: StudioTheme.Values.dialogButtonPadding
                    onClicked: rootView.cancel()
                }

                HelperWidgets.Button {
                    id: applyButton
                    text: qsTr("Ok")
                    leftPadding: StudioTheme.Values.dialogButtonPadding
                    rightPadding: StudioTheme.Values.dialogButtonPadding
                    onClicked: {
                        rootView.apply()
                        rootView.cancel()
                    }
                }
            }
        }
    }
}
