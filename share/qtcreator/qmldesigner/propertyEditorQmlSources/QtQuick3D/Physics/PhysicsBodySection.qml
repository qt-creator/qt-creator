// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

Column {
    width: parent.width

    Section {
        caption: qsTr("Physics Body")
        width: parent.width

        SectionLayout {
            PropertyLabel {
                text: qsTr("Physics Material")
                tooltip: qsTr("The physics material of the body.")
            }

            SecondColumnLayout {
                ItemFilterComboBox {
                    typeFilter: "QtQuick3D.Physics.PhysicsMaterial"
                    backendValue: backendValues.physicsMaterial
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }
}
