// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.15
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Check Box")

    anchors.left: parent.left
    anchors.right: parent.right

    SectionLayout {
        PropertyLabel {
            text: qsTr("Check state")
            tooltip: qsTr("Sets the state of the check box.")
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: backendValues.checkState
                model: [ "Unchecked", "PartiallyChecked", "Checked" ]
                scope: "Qt"
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Tri-state")
            tooltip: qsTr("Toggles if the check box can have an intermediate state.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.tristate.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.tristate
            }

            ExpandingSpacer {}
        }
    }
}
