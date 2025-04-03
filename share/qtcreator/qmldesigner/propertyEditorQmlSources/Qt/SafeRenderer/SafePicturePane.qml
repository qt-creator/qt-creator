// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuickDesignerTheme
import HelperWidgets
import StudioTheme as StudioTheme

Rectangle {
    id: itemPane
    width: 320
    height: 400
    color: Theme.qmlDesignerBackgroundColorDarkAlternate()

    Component.onCompleted: Controller.mainScrollView = mainScrollView

    MouseArea {
        anchors.fill: parent
        onClicked: forceActiveFocus()
    }

    ScrollView {
        id: mainScrollView
        clip: true
        anchors.fill: parent

        Column {
            id: mainColumn
            y: -1
            width: itemPane.width

            onWidthChanged: StudioTheme.Values.responsiveResize(itemPane.width)
            Component.onCompleted: StudioTheme.Values.responsiveResize(itemPane.width)

            ComponentSection {}

            Section {
                caption: qsTr("Safe Picture")
                anchors.left: parent.left
                anchors.right: parent.right

                SectionLayout {
                    PropertyLabel { text: qsTr("Position") }

                    SecondColumnLayout {
                        SpinBox {
                            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                           + StudioTheme.Values.actionIndicatorWidth
                            backendValue: backendValues.x
                            maximumValue: 0xffff
                            minimumValue: -0xffff
                            decimals: 0
                        }

                        Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                        ControlLabel { text: "X" }

                        Spacer { implicitWidth: StudioTheme.Values.controlGap }

                        SpinBox {
                            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                           + StudioTheme.Values.actionIndicatorWidth
                            backendValue: backendValues.y
                            maximumValue: 0xffff
                            minimumValue: -0xffff
                            decimals: 0
                        }

                        Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                        ControlLabel { text: "Y" }

                        ExpandingSpacer {}
                    }

                    PropertyLabel { text: qsTr("Size") }

                    SecondColumnLayout {
                        SpinBox {
                            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                           + StudioTheme.Values.actionIndicatorWidth
                            backendValue: backendValues.width
                            maximumValue: 0xffff
                            minimumValue: 1
                            decimals: 0
                        }

                        Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                        ControlLabel {
                            //: The width of the object
                            text: qsTr("W", "width")
                        }

                        Spacer { implicitWidth: StudioTheme.Values.controlGap }

                        SpinBox {
                            id: heightSpinBox
                            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                           + StudioTheme.Values.actionIndicatorWidth
                            backendValue: backendValues.height
                            maximumValue: 0xffff
                            minimumValue: 1
                            decimals: 0
                        }

                        Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                        ControlLabel {
                            //: The height of the object
                            text: qsTr("H", "height")
                        }

                        ExpandingSpacer {}
                    }

                    PropertyLabel { text: qsTr("Source") }

                    SecondColumnLayout {
                        UrlChooser {
                            backendValue: backendValues.source
                        }

                        ExpandingSpacer {}
                    }

                    PropertyLabel { text: qsTr("Color") }

                    ColorEditor {
                        backendValue: backendValues.color
                        supportGradient: false
                    }

                    PropertyLabel { text: qsTr("fillColor") }

                    ColorEditor {
                        backendValue: backendValues.fillColor
                        supportGradient: false
                    }
                    PropertyLabel { text: qsTr("Opacity") }

                    SecondColumnLayout {
                        SpinBox {
                            implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                           + StudioTheme.Values.actionIndicatorWidth
                            sliderIndicatorVisible: true
                            backendValue: backendValues.opacity
                            decimals: 2
                            minimumValue: 0
                            maximumValue: 1
                            hasSlider: true
                            stepSize: 0.1
                        }

                        ExpandingSpacer {}
                    }
                }
            }
        }
    }
}
