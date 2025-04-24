// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

Section {
    caption: qsTr("Triangle Mesh Shape")
    width: parent.width

    SectionLayout {
        PropertyLabel {
            text: qsTr("Source")
            tooltip: qsTr("Defines the location of the mesh file used to define the shape.")
        }

        SecondColumnLayout {
            UrlChooser {
                id: sourceUrlChooser
                backendValue: backendValues.source
                filter: "*.mesh"
            }

            ExpandingSpacer {}
        }
    }
}
