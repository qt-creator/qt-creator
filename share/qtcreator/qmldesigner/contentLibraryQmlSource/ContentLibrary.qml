// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuickDesignerTheme
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme
import ContentLibraryBackend

Item {
    id: root

    // Called also from C++ to close context menu on focus out
    function closeContextMenu()
    {
        materialsView.closeContextMenu()
        texturesView.closeContextMenu()
        environmentsView.closeContextMenu()
        HelperWidgets.Controller.closeContextMenu()
    }

    // Called from C++
    function clearSearchFilter()
    {
        searchBox.clear();
    }

    Column {
        id: col
        anchors.fill: parent
        spacing: 5

        Rectangle {
            width: parent.width
            height: StudioTheme.Values.doubleToolbarHeight
            color: StudioTheme.Values.themeToolbarBackground

            Column {
                anchors.fill: parent
                anchors.topMargin: 6
                anchors.bottomMargin: 6
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 12

                StudioControls.SearchBox {
                    id: searchBox
                    width: parent.width
                    style: StudioTheme.Values.searchControlStyle
                    enabled: {
                        if (tabBar.currIndex === 0) { // Materials tab
                            ContentLibraryBackend.materialsModel.matBundleExists
                                && ContentLibraryBackend.rootView.hasMaterialLibrary
                                && ContentLibraryBackend.materialsModel.hasRequiredQuick3DImport
                        } else { // Textures / Environments tabs
                            ContentLibraryBackend.texturesModel.texBundleExists
                        }
                    }

                    onSearchChanged: (searchText) => {
                        ContentLibraryBackend.rootView.handleSearchFilterChanged(searchText)

                        // make sure categories with matches are expanded
                        materialsView.expandVisibleSections()
                        texturesView.expandVisibleSections()
                        environmentsView.expandVisibleSections()
                    }
                }

                ContentLibraryTabBar {
                    id: tabBar
                    width: parent.width
                    height: StudioTheme.Values.toolbarHeight
                    tabsModel: [{name: qsTr("Materials"),    icon: StudioTheme.Constants.material_medium},
                                {name: qsTr("Textures"),     icon: StudioTheme.Constants.textures_medium},
                                {name: qsTr("Environments"), icon: StudioTheme.Constants.languageList_medium}]
                }
            }
        }

        UnimportBundleMaterialDialog {
            id: confirmUnimportDialog
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
                model: ContentLibraryBackend.texturesModel
                sectionCategory: "ContentLib_Tex"

                searchBox: searchBox
            }

            ContentLibraryTexturesView {
                id: environmentsView

                width: root.width
                model: ContentLibraryBackend.environmentsModel
                sectionCategory: "ContentLib_Env"

                searchBox: searchBox
            }
        }
    }
}
