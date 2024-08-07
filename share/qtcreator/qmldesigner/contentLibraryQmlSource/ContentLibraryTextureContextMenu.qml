// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme
import ContentLibraryBackend

StudioControls.Menu {
    id: root

    property var targetTexture: null
    property bool hasSceneEnv: false
    property bool showRemoveAction: false // true: adds an option to remove targetTexture

    property bool canUse3D: targetTexture && ContentLibraryBackend.rootView.hasQuick3DImport && ContentLibraryBackend.rootView.hasMaterialLibrary

    function popupMenu(targetTexture = null)
    {
        this.targetTexture = targetTexture
        ContentLibraryBackend.rootView.updateSceneEnvState();
        popup()
    }

    closePolicy: StudioControls.Menu.CloseOnEscape | StudioControls.Menu.CloseOnPressOutside

    StudioControls.MenuItem {
        text: qsTr("Add image")
        enabled: root.targetTexture
        onTriggered: ContentLibraryBackend.rootView.addImage(root.targetTexture)
    }

    StudioControls.MenuItem {
        text: qsTr("Add texture")
        enabled: root.canUse3D
        onTriggered: ContentLibraryBackend.rootView.addTexture(root.targetTexture)
    }

    StudioControls.MenuItem {
        text: qsTr("Add light probe")
        enabled: root.hasSceneEnv && root.canUse3D
        onTriggered: ContentLibraryBackend.rootView.addLightProbe(root.targetTexture)
    }

    StudioControls.MenuItem {
        text: qsTr("Remove from Content Library")
        visible: root.targetTexture && root.showRemoveAction
        height: visible ? implicitHeight : 0
        onTriggered: ContentLibraryBackend.userModel.removeTexture(root.targetTexture)
    }
}
