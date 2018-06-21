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
    property bool useLineEdit: true
    property bool showFormatProperty: false
    property bool showFontSizeMode: false

    SectionLayout {
        columns: 2
        rows: 3
        Label {
            text: qsTr("Text")
        }
        LineEdit {
            //visible: useLineEdit
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
            model: ["NoWrap", "WordWrap", "WrapAnywhere", "WrapAtWordBoundaryOrAnywhere"]
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
            text: qsTr("Font size mode")
            toolTip: qsTr("Specifies how the font size of the displayed text is determined.")
        }
        ComboBox {
            scope: "Text"
            model:  ["FixedSize", "HorizontalFit", "VerticalFit", "Fit"]
            backendValue: backendValues.fontSizeMode
            Layout.fillWidth: true
        }


        Label {
            text: qsTr("Line height")
            tooltip: qsTr("Sets the line height for the text.")
        }

        SpinBox {
            Layout.fillWidth: true
            backendValue: (backendValues.lineHeight === undefined) ? dummyBackendValue : backendValues.lineHeight
            maximumValue: 500
            minimumValue: 0
            decimals: 2
            stepSize: 0.1
        }

    }
}
