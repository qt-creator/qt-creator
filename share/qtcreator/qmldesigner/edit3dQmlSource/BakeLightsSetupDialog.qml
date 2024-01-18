// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Rectangle {
    id: root

    property int toolTipDelay: 1000

    color: StudioTheme.Values.themePanelBackground

    Column {
        id: col
        padding: 5
        leftPadding: 10
        spacing: 5

        Text {
            id: title
            text: qsTr("Lights baking setup for 3D view: %1").arg(sceneId)
            font.bold: true
            font.pixelSize: StudioTheme.Values.myFontSize
            color: StudioTheme.Values.themeTextColor
        }

        Rectangle {
            id: ctrlRect
            width: root.width - 16
            height: root.height - title.height - manualCheckBox.height - 20

            color: StudioTheme.Values.themePanelBackground
            border.color: StudioTheme.Values.themeControlOutline
            border.width: StudioTheme.Values.border

            ListView {
                id: listView

                anchors.fill: parent
                anchors.margins: 4

                clip: true
                model: bakeModel
                spacing: 5

                delegate: Row {
                    spacing: 12
                    enabled: !manualCheckBox.checked

                    Text {
                        text: displayId
                        color: StudioTheme.Values.themeTextColor
                        width: isTitle ? listView.width : listView.width - 320

                        clip: true
                        font.bold: isTitle
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignLeft
                        height: bakeModeCombo.height
                        leftPadding: isTitle ? 0 : 8

                        ToolTipArea {
                            anchors.fill: parent
                            tooltip: displayId // Useful for long ids
                        }
                    }

                    Button {
                        visible: isUnexposed
                        text: qsTr("Expose models and lights")
                        leftPadding: StudioTheme.Values.dialogButtonPadding
                        rightPadding: StudioTheme.Values.dialogButtonPadding
                        height: bakeModeCombo.height
                        onClicked: {
                            rootView.apply()
                            rootView.exposeModelsAndLights(nodeId)
                        }
                    }

                    StudioControls.ComboBox {
                        id: bakeModeCombo
                        model: ListModel {
                            ListElement { text: qsTr("Baking Disabled"); value: "Light.BakeModeDisabled" }
                            ListElement { text: qsTr("Bake Indirect"); value: "Light.BakeModeIndirect" }
                            ListElement { text: qsTr("Bake All"); value: "Light.BakeModeAll" }
                        }

                        visible: !isModel && !isTitle && !isUnexposed
                        textRole: "text"
                        valueRole: "value"
                        currentIndex: bakeMode === "Light.BakeModeAll"
                                      ? 2 : bakeMode === "Light.BakeModeIndirect" ? 1 : 0
                        actionIndicatorVisible: false

                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("The baking mode applied to this light.")
                        ToolTip.delay: root.toolTipDelay

                        onActivated: bakeMode = currentValue
                    }

                    StudioControls.CheckBox {
                        visible: isModel && !isTitle && !isUnexposed
                        checked: inUse
                        text: qsTr("In Use")
                        actionIndicatorVisible: false

                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("If checked, this model contributes to baked lighting,\nfor example in form of casting shadows or indirect light.")
                        ToolTip.delay: root.toolTipDelay

                        onToggled: inUse = checked
                    }

                    StudioControls.CheckBox {
                        visible: isModel && !isTitle && !isUnexposed
                        checked: isEnabled
                        text: qsTr("Enabled")
                        actionIndicatorVisible: false

                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("If checked, baked lightmap texture is generated and rendered for this model.")
                        ToolTip.delay: root.toolTipDelay

                        onToggled: isEnabled = checked
                    }

                    Row {
                        width: resolutionLabel.width + resolutionValue.width + StudioTheme.Values.sectionRowSpacing
                        spacing: StudioTheme.Values.sectionRowSpacing
                        visible: isModel && !isTitle && !isUnexposed
                        Text {
                            id: resolutionLabel
                            text: qsTr("Resolution:")
                            color: enabled ? StudioTheme.Values.themeTextColor
                                           : StudioTheme.Values.themeTextColorDisabled
                            verticalAlignment: Qt.AlignVCenter
                            horizontalAlignment: Qt.AlignRight
                            height: resolutionValue.height
                        }

                        StudioControls.RealSpinBox {
                            id: resolutionValue
                            realFrom: 128
                            realTo: 128000
                            realValue: resolution
                            realStepSize: 128
                            width: 60
                            actionIndicatorVisible: false

                            hoverEnabled: true
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Generated lightmap resolution for this model.")
                            ToolTip.delay: root.toolTipDelay

                            onRealValueChanged: resolution = realValue
                        }
                    }
                }
            }
        }

        Item {
            height: manualCheckBox.height
            width: ctrlRect.width

            StudioControls.CheckBox {
                id: manualCheckBox
                checked: rootView.manualMode
                text: qsTr("Setup baking manually")
                actionIndicatorVisible: false

                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("If checked, baking settings above are not applied on close or bake.\nInstead, user is expected to set baking properties manually.")
                ToolTip.delay: root.toolTipDelay

                onToggled: rootView.manualMode = checked
            }

            Row {
                spacing: StudioTheme.Values.dialogButtonSpacing
                height: manualCheckBox.height
                anchors.right: parent.right

                Button {
                    id: cancelButton
                    text: qsTr("Cancel")
                    leftPadding: StudioTheme.Values.dialogButtonPadding
                    rightPadding: StudioTheme.Values.dialogButtonPadding
                    height: manualCheckBox.height
                    onClicked: rootView.cancel()
                }

                Button {
                    id: applyButton
                    text: qsTr("Apply & Close")
                    leftPadding: StudioTheme.Values.dialogButtonPadding
                    rightPadding: StudioTheme.Values.dialogButtonPadding
                    height: manualCheckBox.height
                    onClicked: {
                        rootView.apply()
                        rootView.cancel()
                    }
                }

                Button {
                    id: bakeButton
                    text: qsTr("Bake")
                    leftPadding: StudioTheme.Values.dialogButtonPadding
                    rightPadding: StudioTheme.Values.dialogButtonPadding
                    height: manualCheckBox.height
                    onClicked: {
                        rootView.apply()
                        rootView.bakeLights()
                    }
                }
            }
        }
    }
}
