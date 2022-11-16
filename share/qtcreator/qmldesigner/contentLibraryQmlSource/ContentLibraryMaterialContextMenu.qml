// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

StudioControls.Menu {
    id: root

    property var targetMaterial: null
    signal unimport(var bundleMat);

    function popupMenu(targetMaterial = null)
    {
        this.targetMaterial = targetMaterial
        popup()
    }

    closePolicy: StudioControls.Menu.CloseOnEscape | StudioControls.Menu.CloseOnPressOutside

    StudioControls.MenuItem {
        text: qsTr("Apply to selected (replace)")
        enabled: root.targetMaterial && materialsModel.hasModelSelection
        onTriggered: materialsModel.applyToSelected(root.targetMaterial, false)
    }

    StudioControls.MenuItem {
        text: qsTr("Apply to selected (add)")
        enabled: root.targetMaterial && materialsModel.hasModelSelection
        onTriggered: materialsModel.applyToSelected(root.targetMaterial, true)
    }

    StudioControls.MenuSeparator {}

    StudioControls.MenuItem {
        enabled: !materialsModel.importerRunning
        text: qsTr("Add an instance to project")

        onTriggered: {
            materialsModel.addToProject(root.targetMaterial)
        }
    }

    StudioControls.MenuItem {
        enabled: !materialsModel.importerRunning && root.targetMaterial && root.targetMaterial.bundleMaterialImported
        text: qsTr("Remove from project")

        onTriggered: root.unimport(root.targetMaterial);
    }
}
