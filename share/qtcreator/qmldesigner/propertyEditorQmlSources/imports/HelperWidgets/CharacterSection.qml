// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Templates 2.15 as T
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Section {
    id: root
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Character")

    property bool showVerticalAlignment: false
    property bool showLineHeight: false
    property bool richTextEditorAvailable: false

    property string fontName: "font"
    property bool showStyle: false

    function getBackendValue(name) {
        return backendValues[root.fontName + "_" + name]
    }

    property variant fontFamily: root.getBackendValue("family")
    property variant pointSize: root.getBackendValue("pointSize")
    property variant pixelSize: root.getBackendValue("pixelSize")

    property variant boldStyle: root.getBackendValue("bold")
    property variant italicStyle: root.getBackendValue("italic")
    property variant underlineStyle: root.getBackendValue("underline")
    property variant strikeoutStyle: root.getBackendValue("strikeout")

    onPointSizeChanged: sizeWidget.setPointPixelSize()
    onPixelSizeChanged: sizeWidget.setPointPixelSize()

    SectionLayout {
        PropertyLabel {
            text: qsTr("Text")
            tooltip: qsTr("Sets the text to display.")
        }

        SecondColumnLayout {
            LineEdit {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: backendValues.text
            }

            Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

            Rectangle {
                id: richTextEditorIndicator
                visible: root.richTextEditorAvailable
                color: "transparent"
                border.color: "transparent"
                implicitWidth: StudioTheme.Values.iconAreaWidth // TODO dedicated value
                implicitHeight: StudioTheme.Values.height // TODO dedicated value

                T.Label {
                    id: richTextEditorIcon
                    anchors.fill: parent
                    text: StudioTheme.Constants.edit
                    color: StudioTheme.Values.themeTextColor
                    font.family: StudioTheme.Constants.iconFont.family
                    font.pixelSize: StudioTheme.Values.myIconFontSize + 4 // TODO
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    states: [
                        State {
                            name: "default"
                            when: !richTextEditorMouseArea.containsMouse
                            PropertyChanges {
                                target: richTextEditorIcon
                                color: StudioTheme.Values.themeLinkIndicatorColor
                            }
                        },
                        State {
                            name: "hover"
                            when: richTextEditorMouseArea.containsMouse
                            PropertyChanges {
                                target: richTextEditorIcon
                                color: StudioTheme.Values.themeLinkIndicatorColorHover
                            }
                        }
                    ]
                }

                MouseArea {
                    id: richTextEditorMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: richTextDialogLoader.show()
                }
            }

            ExpandingSpacer {}

            RichTextEditor {
                onRejected: hideWidget()
                onAccepted: hideWidget()
            }
        }

        PropertyLabel {
            text: qsTr("Font")
            tooltip: qsTr("Sets the font of the text.")
        }

        SecondColumnLayout {
            FontComboBox {
                id: fontComboBox
                property string familyName: backendValue.value
                backendValue: root.fontFamily
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Style name")
            tooltip: qsTr("Sets the style of the selected font. This is prioritized over <b>Weight</b> and <b>Emphasis</b>.")
            enabled: styleNameComboBox.model.length
            blockedByTemplate: !styleNameComboBox.backendValue.isAvailable
        }

        SecondColumnLayout {
            ComboBox {
                id: styleNameComboBox
                property bool styleSet: styleNameComboBox.backendValue.isInModel
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: root.getBackendValue("styleName")
                model: styleNamesForFamily(fontComboBox.familyName)
                valueType: ComboBox.String
                enabled: styleNameComboBox.backendValue.isAvailable && styleNameComboBox.model.length
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Size")
            tooltip: qsTr("Sets the font size in pixels or points.")
        }

        SecondColumnLayout {
            id: sizeWidget
            property bool selectionFlag: selectionChanged

            property bool pixelSize: sizeType.currentText === "px"
            property bool isSetup

            function setPointPixelSize() {
                sizeWidget.isSetup = true
                sizeType.currentIndex = 1

                if (root.pixelSize.isInModel)
                    sizeType.currentIndex = 0

                sizeWidget.isSetup = false
            }

            onSelectionFlagChanged: sizeWidget.setPointPixelSize()

            Item {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                height: sizeSpinBox.height

                SpinBox {
                    id: sizeSpinBox
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    minimumValue: 0
                    visible: !sizeWidget.pixelSize
                    z: !sizeWidget.pixelSize ? 1 : 0
                    maximumValue: 400
                    backendValue: pointSize
                }

                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    minimumValue: 0
                    visible: sizeWidget.pixelSize
                    z: sizeWidget.pixelSize ? 1 : 0
                    maximumValue: 400
                    backendValue: pixelSize
                }
            }

            Spacer {
                implicitWidth: StudioTheme.Values.twoControlColumnGap
                               + StudioTheme.Values.actionIndicatorWidth
            }

            StudioControls.ComboBox {
                id: sizeType
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                width: implicitWidth
                model: ["px", "pt"]
                actionIndicatorVisible: false

                onActivated: {
                    if (sizeWidget.isSetup)
                        return

                    if (sizeType.currentText === "px") {
                        pointSize.resetValue()
                        pixelSize.value = 8
                    } else {
                        pixelSize.resetValue()
                    }
                }
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Text color")
            tooltip: qsTr("Sets the text color.")
        }

        ColorEditor {
            backendValue: backendValues.color
            supportGradient: false
        }

        PropertyLabel {
            text: qsTr("Weight")
            tooltip: qsTr("Sets the overall thickness of the font.")
            enabled: !styleNameComboBox.styleSet
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: root.getBackendValue("weight")
                model: ["Normal", "Light", "ExtraLight", "Thin", "Medium", "DemiBold", "Bold", "ExtraBold", "Black"]
                scope: "Font"
                enabled: !styleNameComboBox.styleSet
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Emphasis")
            tooltip: qsTr("Sets the text to bold, italic, underlined, or strikethrough.")
            enabled: !styleNameComboBox.styleSet
        }

        FontStyleButtons {
            bold: root.boldStyle
            italic: root.italicStyle
            underline: root.underlineStyle
            strikeout: root.strikeoutStyle
            enabled: !styleNameComboBox.styleSet
        }

        PropertyLabel {
            text: qsTr("Alignment H")
            tooltip: qsTr("Sets the horizontal alignment position.")
        }

        SecondColumnLayout {
            AlignmentHorizontalButtons {}

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Alignment V")
            tooltip: qsTr("Sets the vertical alignment position.")
        }

        SecondColumnLayout {
            AlignmentVerticalButtons { visible: root.showVerticalAlignment }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Letter spacing")
            tooltip: qsTr("Sets the letter spacing for the text.")
            blockedByTemplate: !root.getBackendValue("letterSpacing").isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: root.getBackendValue("letterSpacing")
                decimals: 2
                minimumValue: -500
                maximumValue: 500
                stepSize: 0.1
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Word spacing")
            tooltip: qsTr("Sets the word spacing for the text.")
            blockedByTemplate: !root.getBackendValue("wordSpacing").isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: root.getBackendValue("wordSpacing")
                decimals: 2
                minimumValue: -500
                maximumValue: 500
                stepSize: 0.1
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: root.showLineHeight
            text: qsTr("Line height")
            tooltip: qsTr("Sets the line height for the text.")
            blockedByTemplate: !lineHeightSpinBox.enabled
        }

        SecondColumnLayout {
            visible: root.showLineHeight

            SpinBox {
                id: lineHeightSpinBox
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: (backendValues.lineHeight === undefined) ? 1.0
                                                                       : backendValues.lineHeight
                decimals: 2
                minimumValue: 0
                maximumValue: 500
                stepSize: 0.1
                enabled: (backendValues.lineHeight === undefined) ? false
                                                                  : backendValue.isAvailable
            }

            ExpandingSpacer {}
        }
    }

    Loader {
        id: richTextDialogLoader

        visible: false
        active: visible

        function show() {
            richTextDialogLoader.visible = true
        }

        sourceComponent: Item {
            id: richTextEditorParent

            Component.onCompleted: {
                richTextEditor.showWidget()
                richTextEditor.richText = backendValues.text.value
            }

            RichTextEditor {
                id: richTextEditor

                onRejected: {
                    hideWidget()
                    richTextDialogLoader.visible = false
                }
                onAccepted: {
                    backendValues.text.value = richTextEditor.richText
                    backendValues.textFormat.setEnumeration("Text", "RichText")
                    hideWidget()
                    richTextDialogLoader.visible = false
                }
            }
        }
    }
}
