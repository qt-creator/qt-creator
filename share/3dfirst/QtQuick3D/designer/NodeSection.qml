// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Column {
    property int transformSpinBoxMaxWidth: 120
    width: parent.width
    Section {
        id: transformSection
        width: parent.width
        caption: qsTr("Transform")

        ColumnLayout {
            spacing: StudioTheme.Values.transform3DSectionSpacing

            SectionLayout {
                PropertyLabel {
                    text: qsTr("Translation")
                    tooltip: qsTr("Sets the translation of the node.")
                }
                SecondColumnLayout {
                    Row {
                        width: StudioTheme.Values.controlColumnWidth
                        height: parent.height
                        RowLayout {
                            spacing: 25
                            width: StudioTheme.Values.controlColumnWidth

                            SpinBox {
                                id: _spinbox
                                enableValueChangeVisualCue: true
                                spinBoxIndicatorVisible: false
                                ControlLabel {
                                    id: _labelx
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.right: parent.left
                                    anchors.rightMargin: -4
                                    text: "X"

                                    color: StudioTheme.Values.theme3DAxisXColor
                                }

                                minimumValue: -9999999
                                maximumValue: 9999999
                                decimals: 2
                                backendValue: backendValues.x
                                Layout.fillWidth: true
                                Layout.maximumWidth: transformSpinBoxMaxWidth
                                Layout.alignment: Qt.AlignHCenter
                            }
                            SpinBox {
                                enableValueChangeVisualCue: true
                                spinBoxIndicatorVisible: false
                                ControlLabel {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.right: parent.left
                                    anchors.rightMargin: -4
                                    text: "Y"
                                    color: StudioTheme.Values.theme3DAxisYColor
                                }

                                minimumValue: -9999999
                                maximumValue: 9999999
                                decimals: 2
                                backendValue: backendValues.y
                                Layout.fillWidth: true
                                Layout.maximumWidth: transformSpinBoxMaxWidth
                                Layout.alignment: Qt.AlignHCenter
                            }
                            SpinBox {
                                enableValueChangeVisualCue: true
                                spinBoxIndicatorVisible: false
                                ControlLabel {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.right: parent.left
                                    anchors.rightMargin: -4
                                    text: "Z"
                                    color: StudioTheme.Values.theme3DAxisZColor
                                }

                                minimumValue: -9999999
                                maximumValue: 9999999
                                decimals: 2
                                backendValue: backendValues.z

                                Layout.fillWidth: true
                                Layout.maximumWidth: transformSpinBoxMaxWidth
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }
                    }
                }
            }

            SectionLayout {

                PropertyLabel {
                    text: qsTr("Rotation")
                    tooltip: qsTr("Sets the rotation of the node in degrees.")
                }
                SecondColumnLayout {
                    Row {
                        width: StudioTheme.Values.controlColumnWidth
                        height: parent.height
                        RowLayout {
                            spacing: 25
                            width: StudioTheme.Values.controlColumnWidth

                            SpinBox {
                                enableValueChangeVisualCue: true
                                spinBoxIndicatorVisible: false
                                ControlLabel {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.right: parent.left
                                    anchors.rightMargin: -4
                                    text: "X"
                                    color: StudioTheme.Values.theme3DAxisXColor
                                }
                                minimumValue: -9999999
                                maximumValue: 9999999
                                decimals: 2
                                backendValue: backendValues.eulerRotation_x
                                Layout.fillWidth: true
                                Layout.maximumWidth: transformSpinBoxMaxWidth
                                Layout.alignment: Qt.AlignHCenter
                            }
                            SpinBox {
                                enableValueChangeVisualCue: true
                                spinBoxIndicatorVisible: false
                                ControlLabel {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.right: parent.left
                                    anchors.rightMargin: -4
                                    text: "Y"
                                    color: StudioTheme.Values.theme3DAxisYColor
                                }
                                minimumValue: -9999999
                                maximumValue: 9999999
                                decimals: 2
                                backendValue: backendValues.eulerRotation_y
                                Layout.fillWidth: true
                                Layout.maximumWidth: transformSpinBoxMaxWidth
                                Layout.alignment: Qt.AlignHCenter
                            }

                            SpinBox {
                                enableValueChangeVisualCue: true
                                spinBoxIndicatorVisible: false
                                ControlLabel {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.right: parent.left
                                    anchors.rightMargin: -4
                                    text: "X"
                                    color: StudioTheme.Values.theme3DAxisZColor
                                }
                                minimumValue: -9999999
                                maximumValue: 9999999
                                decimals: 2
                                backendValue: backendValues.eulerRotation_z
                                Layout.fillWidth: true
                                Layout.maximumWidth: transformSpinBoxMaxWidth
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }
                    }
                }
            }

            SectionLayout {
                PropertyLabel {
                    text: qsTr("Scale")
                    tooltip: qsTr("Sets the scale of the node.")
                }

                SecondColumnLayout {
                    Row {
                        width: StudioTheme.Values.controlColumnWidth
                        height: parent.height
                        RowLayout {
                            spacing: 25
                            width: StudioTheme.Values.controlColumnWidth

                            SpinBox {
                                enableValueChangeVisualCue: true
                                spinBoxIndicatorVisible: false
                                ControlLabel {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.right: parent.left
                                    anchors.rightMargin: -4
                                    text: "X"
                                    color: StudioTheme.Values.theme3DAxisXColor
                                }
                                minimumValue: -9999999
                                maximumValue: 9999999
                                decimals: 2
                                backendValue: backendValues.scale_x
                                Layout.fillWidth: true
                                Layout.maximumWidth: transformSpinBoxMaxWidth
                                Layout.alignment: Qt.AlignHCenter
                            }
                            SpinBox {
                                enableValueChangeVisualCue: true
                                spinBoxIndicatorVisible: false
                                ControlLabel {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.right: parent.left
                                    anchors.rightMargin: -4
                                    text: "Y"
                                    color: StudioTheme.Values.theme3DAxisYColor
                                }
                                minimumValue: -9999999
                                maximumValue: 9999999
                                decimals: 2
                                backendValue: backendValues.scale_y
                                Layout.fillWidth: true
                                Layout.maximumWidth: transformSpinBoxMaxWidth
                                Layout.alignment: Qt.AlignHCenter
                            }

                            SpinBox {
                                enableValueChangeVisualCue: true
                                spinBoxIndicatorVisible: false
                                ControlLabel {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.right: parent.left
                                    anchors.rightMargin: -4
                                    text: "X"
                                    color: StudioTheme.Values.theme3DAxisZColor
                                }
                                minimumValue: -9999999
                                maximumValue: 9999999
                                decimals: 2
                                backendValue: backendValues.scale_z
                                Layout.fillWidth: true
                                Layout.maximumWidth: transformSpinBoxMaxWidth
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }
                    }
                }
            }
            Item {
                height: 1
            }
            SectionLayout {

                PropertyLabel {
                    text: qsTr("Pivot")
                    tooltip: qsTr("Sets the pivot of the node.")
                }

                SecondColumnLayout {
                    Row {
                        width: StudioTheme.Values.controlColumnWidth
                        height: parent.height
                        RowLayout {
                            spacing: 25
                            width: StudioTheme.Values.controlColumnWidth

                            SpinBox {
                                enableValueChangeVisualCue: true
                                spinBoxIndicatorVisible: false
                                ControlLabel {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.right: parent.left
                                    anchors.rightMargin: -4
                                    text: "X"
                                    color: StudioTheme.Values.theme3DAxisXColor
                                }
                                minimumValue: -9999999
                                maximumValue: 9999999
                                decimals: 2
                                backendValue: backendValues.pivot_x
                                Layout.fillWidth: true
                                Layout.maximumWidth: transformSpinBoxMaxWidth
                                Layout.alignment: Qt.AlignHCenter
                            }
                            SpinBox {
                                enableValueChangeVisualCue: true
                                spinBoxIndicatorVisible: false
                                ControlLabel {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.right: parent.left
                                    anchors.rightMargin: -4
                                    text: "Y"
                                    color: StudioTheme.Values.theme3DAxisYColor
                                }
                                minimumValue: -9999999
                                maximumValue: 9999999
                                decimals: 2
                                backendValue: backendValues.pivot_y
                                Layout.fillWidth: true
                                Layout.maximumWidth: transformSpinBoxMaxWidth
                                Layout.alignment: Qt.AlignHCenter
                            }

                            SpinBox {
                                enableValueChangeVisualCue: true
                                spinBoxIndicatorVisible: false
                                ControlLabel {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.right: parent.left
                                    anchors.rightMargin: -4
                                    text: "X"
                                    color: StudioTheme.Values.theme3DAxisZColor
                                }
                                minimumValue: -9999999
                                maximumValue: 9999999
                                decimals: 2
                                backendValue: backendValues.pivot_z
                                Layout.fillWidth: true
                                Layout.maximumWidth: transformSpinBoxMaxWidth
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }
                    }
                }
            }
        }
    }
    Section {
        width: parent.width
        caption: qsTr("Visibility")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Visibility")
                tooltip: qsTr("Sets the local visibility of the node.")
            }

            SecondColumnLayout {
                // ### should be a slider
                CheckBox {
                    text: qsTr("Visible")
                    backendValue: backendValues.visible
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Opacity")
                tooltip: qsTr("Sets the local opacity value of the node.")
            }

            SecondColumnLayout {
                // ### should be a slider
                SpinBox {
                    minimumValue: 0
                    maximumValue: 1
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.opacity
                    sliderIndicatorVisible: true
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }
}
