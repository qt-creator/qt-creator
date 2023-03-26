// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Container")

    width: parent.width

    SectionLayout {
        PropertyLabel {
            text: qsTr("Current index")
            tooltip: qsTr("Sets the index of the current item.")
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.currentIndex
            }

            ExpandingSpacer {}
        }
    }
}
