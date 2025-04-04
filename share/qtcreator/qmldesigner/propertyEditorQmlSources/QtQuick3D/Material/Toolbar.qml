// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets 2.0
import HelperWidgets 2.0 as HelperWidgets
import StudioTheme as StudioTheme
Rectangle {
    id: root

    property HelperWidgets.QmlMaterialNodeProxy backend: materialNodeBackend

    color: StudioTheme.Values.themeToolbarBackground
    height: StudioTheme.Values.toolbarHeight

    Row {
        id: row
        spacing: StudioTheme.Values.toolbarSpacing
        anchors.verticalCenter: parent.verticalCenter
        leftPadding: 6

        HelperWidgets.AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.apply_medium
            tooltip: qsTr("Apply material to selected model.")
            enabled: has3DModelSelected
            onClicked: root.backend.toolBarAction(QmlMaterialNodeProxy.ApplyToSelected)
        }

        HelperWidgets.AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.create_medium
            tooltip: qsTr("Create new material.")
            onClicked: backend.toolBarAction(QmlMaterialNodeProxy.AddNewMaterial)
        }

        HelperWidgets.AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.delete_medium
            tooltip: qsTr("Delete current material.")
            onClicked: backend.toolBarAction(QmlMaterialNodeProxy.DeleteCurrentMaterial)
        }

        HelperWidgets.AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.materialBrowser_medium
            tooltip: qsTr("Open material browser.")
            onClicked: backend.toolBarAction(QmlMaterialNodeProxy.OpenMaterialBrowser)
        }
    }
}
