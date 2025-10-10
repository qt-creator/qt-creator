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

    enum TabIndex {
        MaterialsTab,
        TexturesTab,
        EffectsTab,
        UserAssetsTab
    }

    // Called also from C++ to close context menu on focus out
    function closeContextMenu() {
        materialsView.closeContextMenu()
        texturesView.closeContextMenu()
        effectsView.closeContextMenu()
        userView.closeContextMenu()
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
                                effectsView.count)

        if (numColumns > maxItems)
            numColumns = maxItems

        let rest = width - (numColumns * root.minThumbSize)
                   - ((numColumns - 1) * StudioTheme.Values.sectionGridSpacing)

        root.thumbnailSize = Math.min(root.minThumbSize + (rest / numColumns),
                                      root.maxThumbSize)
        root.numColumns = numColumns
    }

    Connections {
        target: ContentLibraryBackend.rootView
        function onRequestTab(tabIndex) {
            tabBar.currIndex = tabIndex
        }
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
                        switch (tabBar.currIndex) {
                            case ContentLibrary.TabIndex.MaterialsTab:
                                return ContentLibraryBackend.materialsModel.hasRequiredQuick3DImport
                                    && ContentLibraryBackend.rootView.hasMaterialLibrary
                                    && ContentLibraryBackend.materialsModel.bundleExists
                            case ContentLibrary.TabIndex.TexturesTab:
                                return ContentLibraryBackend.texturesModel.bundleExists
                            case ContentLibrary.TabIndex.EffectsTab:
                                return ContentLibraryBackend.effectsModel.hasRequiredQuick3DImport
                                    && ContentLibraryBackend.effectsModel.bundleExists
                            case ContentLibrary.TabIndex.UserAssetsTab:
                                return !ContentLibraryBackend.userModel.isEmpty
                            default:
                                return false
                        }
                    }

                    onSearchChanged: (searchText) => {
                        compressionTimer.searchText = searchText
                        compressionTimer.restart()
                    }

                    Timer {
                        id: compressionTimer
                        interval: 400
                        running: false
                        repeat: false
                        property string searchText

                        onTriggered: {
                            ContentLibraryBackend.rootView.handleSearchFilterChanged(searchText)

                            materialsView.expandVisibleSections()
                            texturesView.expandVisibleSections()
                            effectsView.expandVisibleSections()
                            userView.expandVisibleSections()
                        }
                    }
                }

                ContentLibraryTabBar {
                    id: tabBar
                    width: parent.width
                    height: StudioTheme.Values.toolbarHeight

                    tabsModel: [
                        { name: qsTr("Materials"),    icon: StudioTheme.Constants.material_medium },
                        { name: qsTr("Textures"),     icon: StudioTheme.Constants.textures_medium },
                        { name: qsTr("Effects"),      icon: StudioTheme.Constants.effects_medium },
                        { name: qsTr("User Assets"),  icon: StudioTheme.Constants.userAssets_medium }
                    ]
                }
            }
        }

        UnimportBundleItemDialog {
            id: confirmUnimportDialog
        }

        DeleteBundleItemDialog {
            id: confirmDeleteDialog
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
                    confirmUnimportDialog.targetBundleLabel = "material"
                    confirmUnimportDialog.targetBundleModel = ContentLibraryBackend.materialsModel
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
                    confirmUnimportDialog.targetBundleLabel = "effect"
                    confirmUnimportDialog.targetBundleModel = ContentLibraryBackend.effectsModel
                    confirmUnimportDialog.open()
                }

                onCountChanged: root.responsiveResize(stackLayout.width, stackLayout.height)
            }

            ContentLibraryUserView {
                id: userView

                adsFocus: root.adsFocus
                width: root.width

                cellWidth: root.thumbnailSize
                cellHeight: root.thumbnailSize + 20
                numColumns: root.numColumns

                searchBox: searchBox

                onUnimport: (bundleItem) => {
                    confirmUnimportDialog.targetBundleItem = bundleItem
                    confirmUnimportDialog.targetBundleLabel = bundleItem.bundleId === "MaterialBundle"
                                                                ? qsTr("material") : qsTr("item")
                    confirmUnimportDialog.targetBundleModel = ContentLibraryBackend.userModel
                    confirmUnimportDialog.open()
                }

                onRemoveFromContentLib: (bundleItem) => {
                    confirmDeleteDialog.targetBundleItem = bundleItem
                    confirmDeleteDialog.targetBundleLabel = bundleItem.bundleId === "MaterialBundle"
                                                                ? qsTr("material") : qsTr("item")
                    confirmDeleteDialog.open()
                }

                onCountChanged: root.responsiveResize(stackLayout.width, stackLayout.height)
            }
        }
    }
}
