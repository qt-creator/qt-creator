// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets 2.0
import StudioTheme as StudioTheme

Rectangle {
    id: root

    property QmlTextureNodeProxy backend: textureNodeBackend

    color: StudioTheme.Values.themeToolbarBackground
    implicitHeight: StudioTheme.Values.toolbarHeight

    Row {
        id: row
        spacing: StudioTheme.Values.toolbarSpacing
        anchors.verticalCenter: parent.verticalCenter
        leftPadding: 6

        AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.apply_medium
            enabled: backend.hasTexture && backend.selectedNodeAcceptsMaterial && hasQuick3DImport && hasMaterialLibrary
            tooltip: qsTr("Apply texture to selected model's material.")
            onClicked: backend.toolbarAction(QmlTextureNodeProxy.ApplyToSelected)
        }

        AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.create_medium
            enabled: hasQuick3DImport && hasMaterialLibrary
            tooltip: qsTr("Create new texture.")
            onClicked: backend.toolbarAction(QmlTextureNodeProxy.AddNewTexture)
        }

        AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.delete_medium
            enabled: backend.hasTexture && hasQuick3DImport && hasMaterialLibrary
            tooltip: qsTr("Delete current texture.")
            onClicked: backend.toolbarAction(QmlTextureNodeProxy.DeleteCurrentTexture)
        }

        AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.materialBrowser_medium
            enabled: backend.hasTexture && hasQuick3DImport && hasMaterialLibrary
            tooltip: qsTr("Open material browser.")
            onClicked: backend.toolbarAction(QmlTextureNodeProxy.OpenMaterialBrowser)
        }
    }
}
