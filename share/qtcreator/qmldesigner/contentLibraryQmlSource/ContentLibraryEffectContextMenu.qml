// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import ContentLibraryBackend

StudioControls.Menu {
    id: root

    property var targetItem: null

    readonly property bool targetAvailable: targetItem && !ContentLibraryBackend.effectsModel.importerRunning

    signal unimport(var bundleEff);

    function popupMenu(targetItem = null)
    {
        this.targetItem = targetItem
        popup()
    }

    closePolicy: StudioControls.Menu.CloseOnEscape | StudioControls.Menu.CloseOnPressOutside

    StudioControls.MenuItem {
        text: qsTr("Add an instance")
        enabled: root.targetAvailable && ContentLibraryBackend.rootView.hasActive3DScene
        onTriggered: ContentLibraryBackend.effectsModel.addInstance(root.targetItem)
    }

    StudioControls.MenuItem {
        enabled: root.targetAvailable && root.targetItem.bundleItemImported
        text: qsTr("Remove from project")

        onTriggered: root.unimport(root.targetItem)
    }
}
