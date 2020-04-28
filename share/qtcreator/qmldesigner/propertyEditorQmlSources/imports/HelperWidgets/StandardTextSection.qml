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
        LineEdit {
            backendValue: backendValues.text
            Layout.fillWidth: true
        }

        Label {
            visible: showVerticalAlignment
            text: qsTr("Wrap mode")
        }

        ComboBox {
            visible: showVerticalAlignment
            Layout.fillWidth: true
            backendValue: backendValues.wrapMode
            scope: "Text"
            model: ["NoWrap", "WordWrap", "WrapAnywhere", "Wrap"]
        }

        Label {
            visible: showElide
            text: qsTr("Elide")
        }

        ComboBox {
            visible: showElide
            Layout.fillWidth: true
            backendValue: backendValues.elide
            scope: "Text"
            model: ["ElideNone", "ElideLeft", "ElideMiddle", "ElideRight"]
        }

        Label {
            visible: showElide
            text: qsTr("Maximum line count")
            tooltip: qsTr("Limits the number of lines that the text item will show.")
        }

        SpinBox {
            visible: showElide
            Layout.fillWidth: true
            backendValue: backendValues.maximumLineCount
            minimumValue: 0
            maximumValue: 10000
            decimals: 0
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
        }
        ComboBox {
            scope: "Text"
            visible: showFormatProperty
            model:  ["PlainText", "RichText", "AutoText"]
            backendValue: backendValues.textFormat
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("Render type")
            toolTip: qsTr("Override the default rendering type for this item.")
        }
        ComboBox {
            scope: "Text"
            model:  ["QtRendering", "NativeRendering"]
            backendValue: backendValues.renderType
            Layout.fillWidth: true
        }

        Label {
            visible: showFontSizeMode
            text: qsTr("Font size mode")
            toolTip: qsTr("Specifies how the font size of the displayed text is determined.")
        }
        ComboBox {
            id: fontSizeMode
            visible: showFontSizeMode
            scope: "Text"
            model:  ["FixedSize", "HorizontalFit", "VerticalFit", "Fit"]
            backendValue: backendValues.fontSizeMode
            Layout.fillWidth: true
        }

        Label {
            visible: showFontSizeMode
            text: qsTr("Minimum size")
        }
        SecondColumnLayout {
            visible: showFontSizeMode

            SpinBox {
                enabled: fontSizeMode.currentIndex !== 0
                minimumValue: 0
                maximumValue: 500
                decimals: 0
                backendValue: backendValues.minimumPixelSize
                Layout.fillWidth: true
                Layout.minimumWidth: 60
            }
            Label {
                text: qsTr("Pixel")
                tooltip: qsTr("Specifies the minimum font pixel size of scaled text.")
                width: 42
            }

            Item {
                width: 4
                height: 4
            }

            SpinBox {
                enabled: fontSizeMode.currentIndex !== 0
                minimumValue: 0
                maximumValue: 500
                decimals: 0
                backendValue: backendValues.minimumPointSize
                Layout.fillWidth: true
                Layout.minimumWidth: 60
            }
            Label {
                text: qsTr("Point")
                tooltip: qsTr("Specifies the minimum font point size of scaled text.")
                width: 42
            }
        }

        Label {
            visible: showLineHeight
            text: qsTr("Line height")
            tooltip: qsTr("Sets the line height for the text.")
        }

        SpinBox {
            visible: showLineHeight
            Layout.fillWidth: true
            backendValue: (backendValues.lineHeight === undefined) ? dummyBackendValue : backendValues.lineHeight
            maximumValue: 500
            minimumValue: 0
            decimals: 2
            stepSize: 0.1
        }

        Label {
            visible: showLineHeight
            text: qsTr("Line height mode")
            toolTip: qsTr("Determines how the line height is specified.")
        }
        ComboBox {
            visible: showLineHeight
            scope: "Text"
            model:  ["ProportionalHeight", "FixedHeight"]
            backendValue: backendValues.lineHeightMode
            Layout.fillWidth: true
        }
    }
}
