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
            height: posIntValue.height + 16

            color: StudioTheme.Values.themePanelBackground
            border.color: StudioTheme.Values.themeControlOutline
            border.width: StudioTheme.Values.border

            Row {
                x: 8
                y: 8
                width: posIntLabel.width + posIntValue.width + StudioTheme.Values.sectionRowSpacing
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

                StudioControls.RealSpinBox {
                    id: posIntValue
                    realFrom: 1
                    realTo: 100000
                    realValue: rootView.posInt
                    realStepSize: 1
                    width: ctrlRect.width - 24 - posIntLabel.width
                    actionIndicatorVisible: false

                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Snap interval for move gizmo.")
                    ToolTip.delay: root.toolTipDelay

                    onRealValueChanged: rootView.posInt = realValue
                }
            }
        }

        Item {
            id: spacer
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
