// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import MaterialBrowserBackend

StudioControls.Menu {
    id: root

    property var targetMaterial: null
    property var targetItem: null
    property int copiedMaterialInternalId: -1
    property var matSectionsModel: []
    property bool restoreFocusOnClose: true

    property var materialBrowserModel: MaterialBrowserBackend.materialBrowserModel

    function popupMenu(targetItem = null, targetMaterial = null)
    {
        this.targetItem = targetItem
        this.targetMaterial = targetMaterial
        restoreFocusOnClose = true
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

            case "SpecularGlossyMaterial":
                root.matSectionsModel = root.matSectionsModel.concat(materialBrowserModel.specularGlossyMaterialSections);
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
        onTriggered: {
            restoreFocusOnClose = false
            root.targetItem.startRename()
        }
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
