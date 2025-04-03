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
                caption: qsTr("Safe Text")
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

                    PropertyLabel { text: qsTr("Text") }

                    SecondColumnLayout {
                        LineEdit {
                            implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                           + StudioTheme.Values.actionIndicatorWidth
                            width: implicitWidth
                            backendValue: backendValues.text
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

                    PropertyLabel { text: qsTr("Font") }

                    SecondColumnLayout {
                        FontComboBox {
                            id: fontComboBox
                            property string familyName: backendValue.value
                            backendValue: backendValues.font_family
                            implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                           + StudioTheme.Values.actionIndicatorWidth
                            width: implicitWidth
                        }

                        ExpandingSpacer {}
                    }

                    PropertyLabel { text: qsTr("Size") }

                    SecondColumnLayout {
                        SpinBox {
                            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                           + StudioTheme.Values.actionIndicatorWidth
                            backendValue: backendValues.font_pixelSize
                            minimumValue: 0
                            maximumValue: 400
                            decimals: 0
                        }

                        Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                        ControlLabel { text: "px" }

                        ExpandingSpacer {}
                    }

                    PropertyLabel { text: qsTr("Emphasis") }

                    SecondColumnLayout {
                        BoolButtonRowButton {
                            id: boldButton
                            buttonIcon: StudioTheme.Constants.fontStyleBold
                            backendValue: backendValues.font_bold
                        }

                        Spacer {
                            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                           + StudioTheme.Values.actionIndicatorWidth
                                           - (boldButton.implicitWidth + italicButton.implicitWidth)
                        }

                        BoolButtonRowButton {
                            id: italicButton
                            buttonIcon: StudioTheme.Constants.fontStyleItalic
                            backendValue: backendValues.font_italic
                        }

                        ExpandingSpacer {}
                    }

                    PropertyLabel { text: qsTr("Alignment H") }

                    SecondColumnLayout {
                        AlignmentHorizontalButtons {
                            scope: "SafeText"
                        }
                        ExpandingSpacer {}
                    }

                    PropertyLabel { text: qsTr("Alignment V") }

                    SecondColumnLayout {
                        AlignmentVerticalButtons {
                            scope: "SafeText"
                        }
                        ExpandingSpacer {}
                    }

                    PropertyLabel { text: qsTr("Wrap mode") }

                    SecondColumnLayout {
                        ComboBox {
                            implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                           + StudioTheme.Values.actionIndicatorWidth
                            width: implicitWidth
                            backendValue: backendValues.wrapMode
                            scope: "SafeText"
                            model: ["NoWrap", "WordWrap", "WrapAnywhere", "Wrap"]
                            enabled: backendValue.isAvailable
                        }
                        ExpandingSpacer {}
                    }

                    PropertyLabel { text: qsTr("Dynamic") }

                    SecondColumnLayout {
                        CheckBox {
                            text: backendValue.valueToString
                            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                           + StudioTheme.Values.actionIndicatorWidth
                            backendValue: backendValues.runtimeEditable
                        }

                        ExpandingSpacer {}
                    }
                }
            }
        }
    }
}
