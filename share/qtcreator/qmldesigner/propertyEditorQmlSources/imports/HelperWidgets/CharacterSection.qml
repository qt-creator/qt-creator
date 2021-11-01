/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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

    property variant fontFamily: getBackendValue("family")
    property variant pointSize: getBackendValue("pointSize")
    property variant pixelSize: getBackendValue("pixelSize")

    property variant boldStyle: getBackendValue("bold")
    property variant italicStyle: getBackendValue("italic")
    property variant underlineStyle: getBackendValue("underline")
    property variant strikeoutStyle: getBackendValue("strikeout")

    onPointSizeChanged: sizeWidget.setPointPixelSize()
    onPixelSizeChanged: sizeWidget.setPointPixelSize()

    SectionLayout {
        PropertyLabel { text: qsTr("Text") }

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

        PropertyLabel { text: qsTr("Font") }

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
            tooltip: qsTr("Font's style.")
            blockedByTemplate: !styleNameComboBox.enabled
        }

        SecondColumnLayout {
            ComboBox {
                id: styleNameComboBox
                property bool styleSet: backendValue.isInModel
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: getBackendValue("styleName")
                model: styleNamesForFamily(fontComboBox.familyName)
                valueType: ComboBox.String
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Size") }

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

        PropertyLabel { text: qsTr("Text color") }

        ColorEditor {
            backendValue: backendValues.color
            supportGradient: false
        }

        PropertyLabel {
            text: qsTr("Weight")
            tooltip: qsTr("Font's weight.")
            enabled: !styleNameComboBox.styleSet
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: getBackendValue("weight")
                model: ["Normal", "Light", "ExtraLight", "Thin", "Medium", "DemiBold", "Bold", "ExtraBold", "Black"]
                scope: "Font"
                enabled: !styleNameComboBox.styleSet
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Emphasis")
            enabled: !styleNameComboBox.styleSet
        }

        FontStyleButtons {
            bold: root.boldStyle
            italic: root.italicStyle
            underline: root.underlineStyle
            strikeout: root.strikeoutStyle
            enabled: !styleNameComboBox.styleSet
        }

        PropertyLabel { text: qsTr("Alignment H") }

        SecondColumnLayout {
            AlignmentHorizontalButtons {}

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Alignment V") }

        SecondColumnLayout {
            AlignmentVerticalButtons { visible: root.showVerticalAlignment }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Letter spacing")
            tooltip: qsTr("Letter spacing for the font.")
            blockedByTemplate: !getBackendValue("letterSpacing").isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: getBackendValue("letterSpacing")
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
            tooltip: qsTr("Word spacing for the font.")
            blockedByTemplate: !getBackendValue("wordSpacing").isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: getBackendValue("wordSpacing")
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
            tooltip: qsTr("Line height for the text.")
            blockedByTemplate: !lineHeightSpinBox.enabled
        }

        SecondColumnLayout {
            visible: root.showLineHeight

            SpinBox {
                id: lineHeightSpinBox
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: (backendValues.lineHeight === undefined) ? dummyBackendValue
                                                                       : backendValues.lineHeight
                decimals: 2
                minimumValue: 0
                maximumValue: 500
                stepSize: 0.1
                enabled: backendValue.isAvailable
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
