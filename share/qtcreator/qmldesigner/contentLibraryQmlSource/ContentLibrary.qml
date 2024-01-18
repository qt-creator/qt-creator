// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import ContentLibraryBackend

Item {
    id: root

    property bool adsFocus: false
    // objectName is used by the dock widget to find this particular ScrollView
    // and set the ads focus on it.
    objectName: "__mainSrollView"

    // Called also from C++ to close context menu on focus out
    function closeContextMenu() {
        materialsView.closeContextMenu()
        texturesView.closeContextMenu()
        environmentsView.closeContextMenu()
        effectsView.closeContextMenu()
        HelperWidgets.Controller.closeContextMenu()
    }

    // Called from C++
    function clearSearchFilter() {
        searchBox.clear()
    }

    property int numColumns: 4
    property real thumbnailSize: 100

    readonly property int minThumbSize: 100
    readonly property int maxThumbSize: 150

    function responsiveResize(width: int, height: int) {
        width -= 2 * StudioTheme.Values.sectionPadding

        let numColumns = Math.floor(width / root.minThumbSize)
        let remainder = width % root.minThumbSize
        let space = (numColumns - 1) * StudioTheme.Values.sectionGridSpacing

        if (remainder < space)
            numColumns -= 1

        if (numColumns < 1)
            return

        let maxItems = Math.max(materialsView.count,
                                texturesView.count,
                                environmentsView.count,
                                effectsView.count)

        if (numColumns > maxItems)
            numColumns = maxItems

        let rest = width - (numColumns * root.minThumbSize)
                   - ((numColumns - 1) * StudioTheme.Values.sectionGridSpacing)

        root.thumbnailSize = Math.min(root.minThumbSize + (rest / numColumns),
                                      root.maxThumbSize)
        root.numColumns = numColumns
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
                anchors.topMargin: StudioTheme.Values.toolbarVerticalMargin
                anchors.bottomMargin: StudioTheme.Values.toolbarVerticalMargin
                anchors.leftMargin: StudioTheme.Values.toolbarHorizontalMargin
                anchors.rightMargin: StudioTheme.Values.toolbarHorizontalMargin
                spacing: StudioTheme.Values.toolbarColumnSpacing

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
                        effectsView.expandVisibleSections()
                    }
                }

                ContentLibraryTabBar {
                    id: tabBar
                    width: parent.width
                    height: StudioTheme.Values.toolbarHeight
                    tabsModel: [{name: qsTr("Materials"),    icon: StudioTheme.Constants.material_medium},
                                {name: qsTr("Textures"),     icon: StudioTheme.Constants.textures_medium},
                                {name: qsTr("Environments"), icon: StudioTheme.Constants.languageList_medium},
                                {name: qsTr("Effects"),      icon: StudioTheme.Constants.effects}]
                }
            }
        }

        UnimportBundleMaterialDialog {
            id: confirmUnimportDialog
        }

        StackLayout {
            id: stackLayout
            width: root.width
            height: root.height - y
            currentIndex: tabBar.currIndex

            onWidthChanged: root.responsiveResize(stackLayout.width, stackLayout.height)

            ContentLibraryMaterialsView {
                id: materialsView

                adsFocus: root.adsFocus
                width: root.width

                cellWidth: root.thumbnailSize
                cellHeight: root.thumbnailSize + 20
                numColumns: root.numColumns
                hideHorizontalScrollBar: true

                searchBox: searchBox

                onUnimport: (bundleMat) => {
                    confirmUnimportDialog.targetBundleItem = bundleMat
                    confirmUnimportDialog.targetBundleType = "material"
                    confirmUnimportDialog.open()
                }

                onCountChanged: root.responsiveResize(stackLayout.width, stackLayout.height)
            }

            ContentLibraryTexturesView {
                id: texturesView

                adsFocus: root.adsFocus
                width: root.width

                cellWidth: root.thumbnailSize
                cellHeight: root.thumbnailSize
                numColumns: root.numColumns
                hideHorizontalScrollBar: true

                model: ContentLibraryBackend.texturesModel
                sectionCategory: "ContentLib_Tex"

                searchBox: searchBox

                onCountChanged: root.responsiveResize(stackLayout.width, stackLayout.height)
            }

            ContentLibraryTexturesView {
                id: environmentsView

                adsFocus: root.adsFocus
                width: root.width

                cellWidth: root.thumbnailSize
                cellHeight: root.thumbnailSize
                numColumns: root.numColumns
                hideHorizontalScrollBar: true

                model: ContentLibraryBackend.environmentsModel
                sectionCategory: "ContentLib_Env"

                searchBox: searchBox

                onCountChanged: root.responsiveResize(stackLayout.width, stackLayout.height)
            }

            ContentLibraryEffectsView {
                id: effectsView

                adsFocus: root.adsFocus
                width: root.width

                cellWidth: root.thumbnailSize
                cellHeight: root.thumbnailSize + 20
                numColumns: root.numColumns
                hideHorizontalScrollBar: true

                searchBox: searchBox

                onUnimport: (bundleItem) => {
                    confirmUnimportDialog.targetBundleItem = bundleItem
                    confirmUnimportDialog.targetBundleType = "effect"
                    confirmUnimportDialog.open()
                }

                onCountChanged: root.responsiveResize(stackLayout.width, stackLayout.height)
            }
        }
    }
}
