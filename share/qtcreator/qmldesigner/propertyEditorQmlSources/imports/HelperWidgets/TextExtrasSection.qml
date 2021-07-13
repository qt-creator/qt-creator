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
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Section {
    id: root
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Text Extras")

    property bool showElide: false
    property bool showWrapMode: false
    property bool showFormatProperty: false
    property bool showFontSizeMode: false
    property bool showLineHeight: false

    SectionLayout {
        PropertyLabel {
            visible: root.showWrapMode
            text: qsTr("Wrap mode")
            blockedByTemplate: !backendValues.wrapMode.isAvailable
        }

        SecondColumnLayout {
            visible: root.showWrapMode

            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: backendValues.wrapMode
                scope: "Text"
                model: ["NoWrap", "WordWrap", "WrapAnywhere", "Wrap"]
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: root.showElide
            text: qsTr("Elide")
            blockedByTemplate: !backendValues.elide.isAvailable
        }

        SecondColumnLayout {
            visible: root.showElide

            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: backendValues.elide
                scope: "Text"
                model: ["ElideNone", "ElideLeft", "ElideMiddle", "ElideRight"]
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: root.showFormatProperty
            text: qsTr("Format")
            blockedByTemplate: !backendValues.textFormat.isAvailable
        }

        SecondColumnLayout {
            visible: root.showFormatProperty

            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                scope: "Text"
                model: ["PlainText", "RichText", "AutoText"]
                backendValue: backendValues.textFormat
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Render type")
            tooltip: qsTr("Overrides the default rendering type for this component.")
            blockedByTemplate: !backendValues.renderType.isAvailable
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                scope: "Text"
                model: ["QtRendering", "NativeRendering"]
                backendValue: backendValues.renderType
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: root.showLineHeight
            text: qsTr("Line height mode")
            tooltip: qsTr("Determines how the line height is specified.")
            blockedByTemplate: !backendValues.lineHeightMode.isAvailable
        }

        SecondColumnLayout {
            visible: root.showLineHeight

            ComboBox {
                scope: "Text"
                model: ["ProportionalHeight", "FixedHeight"]
                backendValue: backendValues.lineHeightMode
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: root.showFontSizeMode
            text: qsTr("Size mode")
            tooltip: qsTr("Specifies how the font size of the displayed text is determined.")
            blockedByTemplate: !backendValues.fontSizeMode.isAvailable
        }

        SecondColumnLayout {
            visible: root.showFontSizeMode

            ComboBox {
                id: fontSizeMode
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                scope: "Text"
                model: ["FixedSize", "HorizontalFit", "VerticalFit", "Fit"]
                backendValue: backendValues.fontSizeMode
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: root.showFontSizeMode
            text: qsTr("Min size")
            blockedByTemplate: !backendValues.minimumPixelSize.isAvailable
                               && !backendValues.minimumPointSize.isAvailable
        }

        SecondColumnLayout {
            visible: root.showFontSizeMode

            SpinBox {
                enabled: (fontSizeMode.currentIndex !== 0) || backendValue.isAvailable
                minimumValue: 0
                maximumValue: 500
                decimals: 0
                backendValue: backendValues.minimumPixelSize
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                text: "px"
                tooltip: qsTr("Minimum font pixel size of scaled text.")
                enabled: backendValues.minimumPixelSize.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                enabled: (fontSizeMode.currentIndex !== 0) || backendValue.isAvailable
                minimumValue: 0
                maximumValue: 500
                decimals: 0
                backendValue: backendValues.minimumPointSize
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                text: "pt"
                tooltip: qsTr("Minimum font point size of scaled text.")
                enabled: backendValues.minimumPointSize.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: root.showElide
            text: qsTr("Max line count")
            tooltip: qsTr("Limits the number of lines that the text component will show.")
            blockedByTemplate: !backendValues.maximumLineCount.isAvailable
        }

        SecondColumnLayout {
            visible: root.showElide

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.maximumLineCount
                minimumValue: 0
                maximumValue: 10000
                decimals: 0
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }
    }
}
