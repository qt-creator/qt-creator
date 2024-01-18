// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

Section {
    id: section
    caption: qsTr("Button")

    anchors.left: parent.left
    anchors.right: parent.right

    SectionLayout {
        PropertyLabel {
            text: qsTr("Appearance")
            tooltip: qsTr("Toggles if the button is flat or highlighted.")
            blockedByTemplate: !backendValues.flat.isAvailable
                               && !backendValues.highlighted.isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                text: qsTr("Flat")
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.flat
                enabled: backendValue.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

            CheckBox {
                text: qsTr("Highlight")
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.highlighted
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }
    }
}
