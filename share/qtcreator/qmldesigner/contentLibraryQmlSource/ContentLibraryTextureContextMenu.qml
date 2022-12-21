// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

StudioControls.Menu {
    id: root

    property var targetTexture: null
    property bool hasSceneEnv: false

    property bool canUse3D: targetTexture && rootView.hasQuick3DImport && rootView.hasMaterialLibrary

    function popupMenu(targetTexture = null)
    {
        this.targetTexture = targetTexture
        rootView.updateSceneEnvState();
        popup()
    }

    closePolicy: StudioControls.Menu.CloseOnEscape | StudioControls.Menu.CloseOnPressOutside

    StudioControls.MenuItem {
        text: qsTr("Add image")
        enabled: root.targetTexture
        onTriggered: rootView.addImage(root.targetTexture)
    }

    StudioControls.MenuItem {
        text: qsTr("Add texture")
        enabled: canUse3D
        onTriggered: rootView.addTexture(root.targetTexture)
    }

    StudioControls.MenuItem {
        text: qsTr("Add light probe")
        enabled: root.hasSceneEnv && canUse3D
        onTriggered: rootView.addLightProbe(root.targetTexture)
    }
}
