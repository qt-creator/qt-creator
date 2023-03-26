// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import MaterialBrowserBackend

StudioControls.Menu {
    id: root

    property var targetTexture: null
    property int copiedTextureInternalId: -1

    property var materialBrowserTexturesModel: MaterialBrowserBackend.materialBrowserTexturesModel

    function popupMenu(targetTexture = null)
    {
        this.targetTexture = targetTexture
        materialBrowserTexturesModel.updateSceneEnvState()
        materialBrowserTexturesModel.updateModelSelectionState()
        popup()
    }

    closePolicy: StudioControls.Menu.CloseOnEscape | StudioControls.Menu.CloseOnPressOutside

    StudioControls.MenuItem {
        text: qsTr("Apply to selected model")
        enabled: root.targetTexture && materialBrowserTexturesModel.hasSingleModelSelection
        onTriggered: materialBrowserTexturesModel.applyToSelectedModel(root.targetTexture.textureInternalId)
    }

    StudioControls.MenuItem {
        text: qsTr("Apply to selected material")
        enabled: root.targetTexture && MaterialBrowserBackend.materialBrowserModel.selectedIndex >= 0
        onTriggered: materialBrowserTexturesModel.applyToSelectedMaterial(root.targetTexture.textureInternalId)
    }

    StudioControls.MenuItem {
        text: qsTr("Apply as light probe")
        enabled: root.targetTexture && materialBrowserTexturesModel.hasSceneEnv
        onTriggered: materialBrowserTexturesModel.applyAsLightProbe(root.targetTexture.textureInternalId)
    }

    StudioControls.MenuSeparator {}

    StudioControls.MenuItem {
        text: qsTr("Duplicate")
        enabled: root.targetTexture
        onTriggered: materialBrowserTexturesModel.duplicateTexture(materialBrowserTexturesModel.selectedIndex)
    }

    StudioControls.MenuItem {
        text: qsTr("Delete")
        enabled: root.targetTexture
        onTriggered: materialBrowserTexturesModel.deleteTexture(materialBrowserTexturesModel.selectedIndex)
    }

    StudioControls.MenuSeparator {}

    StudioControls.MenuItem {
        text: qsTr("Create New Texture")
        onTriggered: materialBrowserTexturesModel.addNewTexture()
    }
}
