// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Baked Lightmap")
    width: parent.width

    SectionLayout {
        PropertyLabel {
            text: qsTr("Enabled")
            tooltip: qsTr("When false, the lightmap generated for the model is not stored during lightmap baking,\neven if the key is set to a non-empty value.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.enabled.valueToString
                backendValue: backendValues.enabled
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Key")
            tooltip: qsTr("Sets the filename base for baked lightmap files for the model.\nNo other Model in the scene can use the same key.")
        }

        SecondColumnLayout {
            LineEdit {
                backendValue: backendValues.key
                showTranslateCheckBox: false
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Load Prefix")
            tooltip: qsTr("Sets the folder where baked lightmap files are generated.\nIt should be a relative path.")
        }

        SecondColumnLayout {
            LineEdit {
                backendValue: backendValues.loadPrefix
                showTranslateCheckBox: false
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
            }

            ExpandingSpacer {}
        }
    }
}
