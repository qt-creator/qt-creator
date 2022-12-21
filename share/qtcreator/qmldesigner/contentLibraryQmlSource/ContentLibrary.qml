// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuickDesignerTheme
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Item {
    id: root

    // Called also from C++ to close context menu on focus out
    function closeContextMenu()
    {
        materialsView.closeContextMenu()
        texturesView.closeContextMenu()
        environmentsView.closeContextMenu()
    }

    // Called from C++
    function clearSearchFilter()
    {
        searchBox.clear();
    }

    Column {
        id: col
        y: 5
        spacing: 5

        StudioControls.SearchBox {
            id: searchBox

            width: root.width
            enabled: {
                if (tabBar.currIndex == 0) { // Materials tab
                    materialsModel.matBundleExists
                            && rootView.hasMaterialLibrary
                            && materialsModel.hasRequiredQuick3DImport
                } else { // Textures / Environments tabs
                    texturesModel.texBundleExists
                }
            }

            onSearchChanged: (searchText) => {
                rootView.handleSearchFilterChanged(searchText)

                // make sure categories with matches are expanded
                materialsView.expandVisibleSections()
                texturesView.expandVisibleSections()
                environmentsView.expandVisibleSections()
            }
        }

        UnimportBundleMaterialDialog {
            id: confirmUnimportDialog
        }

        ContentLibraryTabBar {
            id: tabBar
             // TODO: update icons
            tabsModel: [{name: qsTr("Materials"),    icon: StudioTheme.Constants.gradient},
                        {name: qsTr("Textures"),     icon: StudioTheme.Constants.materialPreviewEnvironment},
                        {name: qsTr("Environments"), icon: StudioTheme.Constants.translationSelectLanguages}]
        }

        StackLayout {
            width: root.width
            height: root.height - y
            currentIndex: tabBar.currIndex

            ContentLibraryMaterialsView {
                id: materialsView

                width: root.width

                searchBox: searchBox

                onUnimport: (bundleMat) => {
                    confirmUnimportDialog.targetBundleMaterial = bundleMat
                    confirmUnimportDialog.open()
                }
            }

            ContentLibraryTexturesView {
                id: texturesView

                width: root.width
                model: texturesModel

                searchBox: searchBox
            }

            ContentLibraryTexturesView {
                id: environmentsView

                width: root.width
                model: environmentsModel

                searchBox: searchBox
            }
        }
    }
}
