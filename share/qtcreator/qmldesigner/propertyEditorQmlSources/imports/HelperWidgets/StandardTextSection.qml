/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

import QtQuick 2.1
import HelperWidgets 2.0
import QtQuick.Layouts 1.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Text")

    property bool showIsWrapping: false
    property bool showElide: false
    property bool showVerticalAlignment: false
    property bool showFormatProperty: false
    property bool showFontSizeMode: false
    property bool showLineHeight: false


    SectionLayout {
        columns: 2
        rows: 3
        Label {
            text: qsTr("Text")
        }

        RowLayout {
            LineEdit {
                backendValue: backendValues.text
                Layout.fillWidth: true
            }

            StudioControls.AbstractButton {
                id: richTextEditorButton
                buttonIcon: StudioTheme.Constants.edit
                onClicked: {
                    richTextDialogLoader.show()
                }
            }

            RichTextEditor{
                onRejected: {
                    hideWidget()
                }
                onAccepted: {
                    hideWidget()
                }
            }
        }

        Label {
            visible: showVerticalAlignment
            text: qsTr("Wrap mode")
            disabledState: !backendValues.wrapMode.isAvailable
        }

        ComboBox {
            visible: showVerticalAlignment
            Layout.fillWidth: true
            backendValue: backendValues.wrapMode
            scope: "Text"
            model: ["NoWrap", "WordWrap", "WrapAnywhere", "Wrap"]
            enabled: backendValue.isAvailable
        }

        Label {
            visible: showElide
            text: qsTr("Elide")
            disabledState: !backendValues.elide.isAvailable
        }

        ComboBox {
            visible: showElide
            Layout.fillWidth: true
            backendValue: backendValues.elide
            scope: "Text"
            model: ["ElideNone", "ElideLeft", "ElideMiddle", "ElideRight"]
            enabled: backendValue.isAvailable
        }

        Label {
            visible: showElide
            text: qsTr("Maximum line count")
            tooltip: qsTr("Limits the number of lines that the text item will show.")
            disabledState: !backendValues.maximumLineCount.isAvailable
        }

        SpinBox {
            visible: showElide
            Layout.fillWidth: true
            backendValue: backendValues.maximumLineCount
            minimumValue: 0
            maximumValue: 10000
            decimals: 0
            enabled: backendValue.isAvailable
        }

        Label {
            text: qsTr("Alignment")
        }

        AligmentHorizontalButtons {

        }

        Label {
            visible: showVerticalAlignment
            text: ("")
        }

        AligmentVerticalButtons {
            visible: showVerticalAlignment
        }


        Label {
            visible: showFormatProperty
            text: qsTr("Format")
            disabledState: !backendValues.textFormat.isAvailable
        }
        ComboBox {
            scope: "Text"
            visible: showFormatProperty
            model:  ["PlainText", "RichText", "AutoText"]
            backendValue: backendValues.textFormat
            Layout.fillWidth: true
            enabled: backendValue.isAvailable
        }

        Label {
            text: qsTr("Render type")
            toolTip: qsTr("Overrides the default rendering type for this item.")
            disabledState: !backendValues.renderType.isAvailable
        }
        ComboBox {
            scope: "Text"
            model:  ["QtRendering", "NativeRendering"]
            backendValue: backendValues.renderType
            Layout.fillWidth: true
            enabled: backendValue.isAvailable
        }

        Label {
            visible: showFontSizeMode
            text: qsTr("Font size mode")
            toolTip: qsTr("Specifies how the font size of the displayed text is determined.")
            disabledState: !backendValues.fontSizeMode.isAvailable
        }
        ComboBox {
            id: fontSizeMode
            visible: showFontSizeMode
            scope: "Text"
            model:  ["FixedSize", "HorizontalFit", "VerticalFit", "Fit"]
            backendValue: backendValues.fontSizeMode
            Layout.fillWidth: true
            enabled: backendValue.isAvailable
        }

        Label {
            visible: showFontSizeMode
            text: qsTr("Minimum size")
            disabledState: !backendValues.minimumPixelSize.isAvailable
                           && !backendValues.minimumPointSize.isAvailable
        }
        SecondColumnLayout {
            visible: showFontSizeMode

            SpinBox {
                enabled: (fontSizeMode.currentIndex !== 0) || backendValue.isAvailable
                minimumValue: 0
                maximumValue: 500
                decimals: 0
                backendValue: backendValues.minimumPixelSize
                Layout.fillWidth: true
                Layout.minimumWidth: 60
            }
            Label {
                text: qsTr("Pixel")
                tooltip: qsTr("Minimum font pixel size of scaled text.")
                width: 42
                disabledStateSoft: !backendValues.minimumPixelSize.isAvailable
            }

            Item {
                width: 4
                height: 4
            }

            SpinBox {
                enabled: (fontSizeMode.currentIndex !== 0) || backendValue.isAvailable
                minimumValue: 0
                maximumValue: 500
                decimals: 0
                backendValue: backendValues.minimumPointSize
                Layout.fillWidth: true
                Layout.minimumWidth: 60
            }
            Label {
                text: qsTr("Point")
                tooltip: qsTr("Minimum font point size of scaled text.")
                width: 42
                disabledStateSoft: !backendValues.minimumPointSize.isAvailable
            }
        }

        Label {
            visible: showLineHeight
            text: qsTr("Line height")
            tooltip: qsTr("Line height for the text.")
            disabledState: !lineHeightSpinBox.enabled
        }

        SpinBox {
            id: lineHeightSpinBox
            visible: showLineHeight
            Layout.fillWidth: true
            backendValue: (backendValues.lineHeight === undefined) ? dummyBackendValue : backendValues.lineHeight
            maximumValue: 500
            minimumValue: 0
            decimals: 2
            stepSize: 0.1
            enabled: backendValue.isAvailable
        }

        Label {
            visible: showLineHeight
            text: qsTr("Line height mode")
            toolTip: qsTr("Determines how the line height is specified.")
            disabledState: !backendValues.lineHeightMode.isAvailable
        }
        ComboBox {
            visible: showLineHeight
            scope: "Text"
            model:  ["ProportionalHeight", "FixedHeight"]
            backendValue: backendValues.lineHeightMode
            Layout.fillWidth: true
            enabled: backendValue.isAvailable
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
