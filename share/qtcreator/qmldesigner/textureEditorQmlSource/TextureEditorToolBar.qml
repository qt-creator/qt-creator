// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme
import TextureToolBarAction

Rectangle {
    id: root

    color: StudioTheme.Values.themeToolbarBackground
    height: StudioTheme.Values.toolbarHeight
    width: row.width

    signal toolBarAction(int action)

    Row {
        id: row
        spacing: StudioTheme.Values.toolbarSpacing
        anchors.verticalCenter: parent.verticalCenter
        leftPadding: 6

        HelperWidgets.AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.apply_medium
            enabled: hasTexture && hasSingleModelSelection && hasQuick3DImport && hasMaterialLibrary
            tooltip: qsTr("Apply texture to selected model's material.")
            onClicked: root.toolBarAction(ToolBarAction.ApplyToSelected)
        }

        HelperWidgets.AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.create_medium
            enabled: hasQuick3DImport && hasMaterialLibrary
            tooltip: qsTr("Create new texture.")
            onClicked: root.toolBarAction(ToolBarAction.AddNewTexture)
        }

        HelperWidgets.AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.delete_medium
            enabled: hasTexture && hasQuick3DImport && hasMaterialLibrary
            tooltip: qsTr("Delete current texture.")
            onClicked: root.toolBarAction(ToolBarAction.DeleteCurrentTexture)
        }

        HelperWidgets.AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.materialBrowser_medium
            enabled: hasTexture && hasQuick3DImport && hasMaterialLibrary
            tooltip: qsTr("Open material browser.")
            onClicked: root.toolBarAction(ToolBarAction.OpenMaterialBrowser)
        }
    }
}
