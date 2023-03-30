// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme
import MaterialToolBarAction 1.0

Rectangle {
    id: root

    color: StudioTheme.Values.themeToolbarBackground
    height: StudioTheme.Values.toolbarHeight

    signal toolBarAction(int action)

    Row {
        id: row
        spacing: StudioTheme.Values.toolbarSpacing
        anchors.verticalCenter: parent.verticalCenter
        leftPadding: 6

        HelperWidgets.AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.apply_medium
            enabled: hasMaterial && hasModelSelection && hasQuick3DImport && hasMaterialLibrary
            tooltip: qsTr("Apply material to selected model.")
            onClicked: root.toolBarAction(ToolBarAction.ApplyToSelected)
        }

        HelperWidgets.AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.create_medium
            enabled: hasQuick3DImport && hasMaterialLibrary
            tooltip: qsTr("Create new material.")
            onClicked: root.toolBarAction(ToolBarAction.AddNewMaterial)
        }

        HelperWidgets.AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.delete_medium
            enabled: hasMaterial && hasQuick3DImport && hasMaterialLibrary
            tooltip: qsTr("Delete current material.")
            onClicked: root.toolBarAction(ToolBarAction.DeleteCurrentMaterial)
        }

        HelperWidgets.AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.materialBrowser_medium
            enabled: hasMaterial && hasQuick3DImport && hasMaterialLibrary
            tooltip: qsTr("Open material browser.")
            onClicked: root.toolBarAction(ToolBarAction.OpenMaterialBrowser)
        }
    }
}
