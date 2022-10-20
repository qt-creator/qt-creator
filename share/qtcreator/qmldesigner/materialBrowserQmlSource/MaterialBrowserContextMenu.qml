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

import QtQuick
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

StudioControls.Menu {
    id: root

    property var targetMaterial: null
    property var targetItem: null
    property int copiedMaterialInternalId: -1
    property var matSectionsModel: []

    function popupMenu(targetItem = null, targetMaterial = null)
    {
        this.targetItem = targetItem
        this.targetMaterial = targetMaterial
        popup()
    }

    closePolicy: StudioControls.Menu.CloseOnEscape | StudioControls.Menu.CloseOnPressOutside

    StudioControls.MenuItem {
        text: qsTr("Apply to selected (replace)")
        enabled: root.targetMaterial && materialBrowserModel.hasModelSelection
        onTriggered: materialBrowserModel.applyToSelected(root.targetMaterial.materialInternalId, false)
    }

    StudioControls.MenuItem {
        text: qsTr("Apply to selected (add)")
        enabled: root.targetMaterial && materialBrowserModel.hasModelSelection
        onTriggered: materialBrowserModel.applyToSelected(root.targetMaterial.materialInternalId, true)
    }

    StudioControls.MenuSeparator {}

    StudioControls.Menu {
        title: qsTr("Copy properties")
        enabled: root.targetMaterial

        width: parent.width

        onAboutToShow: {
            if (root.targetMaterial.hasDynamicProperties)
                root.matSectionsModel = ["All", "Custom"];
            else
                root.matSectionsModel = ["All"];

            switch (root.targetMaterial.materialType) {
            case "DefaultMaterial":
                root.matSectionsModel = root.matSectionsModel.concat(materialBrowserModel.defaultMaterialSections);
                break;

            case "PrincipledMaterial":
                root.matSectionsModel = root.matSectionsModel.concat(materialBrowserModel.principledMaterialSections);
                break;

            case "CustomMaterial":
                root.matSectionsModel = root.matSectionsModel.concat(materialBrowserModel.customMaterialSections);
                break;
            }
        }

        Repeater {
            model: root.matSectionsModel

            StudioControls.MenuItem {
                text: modelData
                enabled: root.targetMaterial
                onTriggered: {
                    root.copiedMaterialInternalId = root.targetMaterial.materialInternalId
                    materialBrowserModel.copyMaterialProperties(materialBrowserModel.selectedIndex, modelData)
                }
            }
        }
    }

    StudioControls.MenuItem {
        text: qsTr("Paste properties")
        enabled: root.targetMaterial
                 && root.copiedMaterialInternalId !== root.targetMaterial.materialInternalId
                 && root.targetMaterial.materialType === materialBrowserModel.copiedMaterialType
                 && materialBrowserModel.isCopiedMaterialValid()
        onTriggered: materialBrowserModel.pasteMaterialProperties(materialBrowserModel.selectedIndex)
    }

    StudioControls.MenuSeparator {}

    StudioControls.MenuItem {
        text: qsTr("Duplicate")
        enabled: root.targetMaterial
        onTriggered: materialBrowserModel.duplicateMaterial(materialBrowserModel.selectedIndex)
    }

    StudioControls.MenuItem {
        text: qsTr("Rename")
        enabled: root.targetItem
        onTriggered: root.targetItem.startRename();
    }

    StudioControls.MenuItem {
        text: qsTr("Delete")
        enabled: root.targetMaterial

        onTriggered: materialBrowserModel.deleteMaterial(materialBrowserModel.selectedIndex)
    }

    StudioControls.MenuSeparator {}

    StudioControls.MenuItem {
        text: qsTr("Create New Material")

        onTriggered: materialBrowserModel.addNewMaterial()
    }
}
