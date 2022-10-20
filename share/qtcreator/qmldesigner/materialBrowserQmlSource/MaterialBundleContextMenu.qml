/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
        enabled: root.targetMaterial && materialBrowserModel.hasModelSelection
        onTriggered: materialBrowserBundleModel.applyToSelected(root.targetMaterial, false)
    }

    StudioControls.MenuItem {
        text: qsTr("Apply to selected (add)")
        enabled: root.targetMaterial && materialBrowserModel.hasModelSelection
        onTriggered: materialBrowserBundleModel.applyToSelected(root.targetMaterial, true)
    }

    StudioControls.MenuSeparator {}

    StudioControls.MenuItem {
        enabled: !materialBrowserBundleModel.importerRunning
        text: qsTr("Add an instance to project")

        onTriggered: {
            materialBrowserBundleModel.addToProject(root.targetMaterial)
        }
    }

    StudioControls.MenuItem {
        enabled: !materialBrowserBundleModel.importerRunning && root.targetMaterial.bundleMaterialImported
        text: qsTr("Remove from project")

        onTriggered: root.unimport(root.targetMaterial);
    }
}
