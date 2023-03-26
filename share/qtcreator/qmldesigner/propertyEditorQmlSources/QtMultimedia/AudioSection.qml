// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Audio")

    anchors.left: parent.left
    anchors.right: parent.right

    SectionLayout {
        PropertyLabel { text: qsTr("Volume") }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.volume
                decimals: 1
                minimumValue: 0.0
                maximumValue: 1.0
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Muted") }

        SecondColumnLayout {
            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.muted
                text: backendValues.muted.valueToString
            }

            ExpandingSpacer {}
        }
    }
}
