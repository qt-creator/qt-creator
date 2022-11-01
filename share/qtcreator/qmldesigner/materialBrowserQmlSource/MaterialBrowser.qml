// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Item {
    id: root

    readonly property int cellWidth: 100
    readonly property int cellHeight: 120

    property var currMaterialItem: null

    // Called also from C++ to close context menu on focus out
    function closeContextMenu()
    {
        ctxMenu.close()
        ctxMenuBundle.close()
    }

    // Called from C++ to refresh a preview material after it changes
    function refreshPreview(idx)
    {
        var item = gridRepeater.itemAt(idx);
        if (item)
            item.refreshPreview();
    }

    // Called from C++
    function clearSearchFilter()
    {
        searchBox.clear();
    }

    MouseArea {
        id: rootMouseArea

        anchors.fill: parent

        acceptedButtons: Qt.RightButton

        onClicked: (mouse) => {
            // root context-menu works only for user materials
            var userMatsSecBottom = mapFromItem(userMaterialsSection, 0, userMaterialsSection.y).y
                                    + userMaterialsSection.height;

            if (!materialBrowserModel.hasMaterialRoot && materialBrowserModel.hasQuick3DImport
                && (!materialBrowserBundleModel.matBundleExists || mouse.y < userMatsSecBottom)) {
                ctxMenu.popupMenu()
            }
        }
    }

    Connections {
        target: materialBrowserModel

        function onSelectedIndexChanged() {
            // commit rename upon changing selection
            if (root.currMaterialItem)
                root.currMaterialItem.commitRename();

            root.currMaterialItem = gridRepeater.itemAt(materialBrowserModel.selectedIndex);
        }
    }

    MaterialBrowserContextMenu {
        id: ctxMenu
    }

    MaterialBundleContextMenu {
        id: ctxMenuBundle

        onUnimport: (bundleMat) => {
            unimportBundleMaterialDialog.targetBundleMaterial = bundleMat
            unimportBundleMaterialDialog.open()
        }
    }

    UnimportBundleMaterialDialog {
        id: unimportBundleMaterialDialog
    }

    Column {
        id: col
        y: 5
        spacing: 5

        Row {
            width: root.width
            enabled: !materialBrowserModel.hasMaterialRoot && materialBrowserModel.hasQuick3DImport

            StudioControls.SearchBox {
                id: searchBox

                width: root.width - addMaterialButton.width

                onSearchChanged: (searchText) => {
                    rootView.handleSearchFilterChanged(searchText)

                    // make sure searched categories that have matches are expanded
                    if (!materialBrowserModel.isEmpty && !userMaterialsSection.expanded)
                        userMaterialsSection.expanded = true

                    if (!materialBrowserBundleModel.isEmpty && !bundleMaterialsSection.expanded)
                        bundleMaterialsSection.expanded = true

                    for (let i = 0; i < bundleMaterialsSectionRepeater.count; ++i) {
                        let sec = bundleMaterialsSectionRepeater.itemAt(i)
                        if (sec.visible && !sec.expanded)
                            sec.expanded = true
                    }
                }
            }

            IconButton {
                id: addMaterialButton

                tooltip: qsTr("Add a material.")

                icon: StudioTheme.Constants.plus
                anchors.verticalCenter: parent.verticalCenter
                buttonSize: searchBox.height
                onClicked: materialBrowserModel.addNewMaterial()
                enabled: materialBrowserModel.hasQuick3DImport
            }
        }

        Text {
            text: {
                if (materialBrowserModel.hasMaterialRoot)
                    qsTr("<b>Material Browser</b> is disabled inside a material component.")
                else if (!materialBrowserModel.hasQuick3DImport)
                    qsTr("To use <b>Material Browser</b>, first add the QtQuick3D module in the <b>Components</b> view.")
                else
                    ""
            }

            textFormat: Text.RichText
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.mediumFontSize
            topPadding: 30
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            width: root.width
            visible: text !== ""
        }

        ScrollView {
            id: scrollView

            width: root.width
            height: root.height - searchBox.height
            clip: true
            visible: materialBrowserModel.hasQuick3DImport && !materialBrowserModel.hasMaterialRoot
            interactive: !ctxMenu.opened && !ctxMenuBundle.opened

            Column {
                Section {
                    id: userMaterialsSection

                    width: root.width
                    caption: qsTr("Materials")
                    hideHeader: !materialBrowserBundleModel.matBundleExists
                    dropEnabled: true

                    onDropEnter: (drag) => {
                        drag.accepted = rootView.draggedBundleMaterial
                        userMaterialsSection.highlight = rootView.draggedBundleMaterial
                    }

                    onDropExit: {
                        userMaterialsSection.highlight = false
                    }

                    onDrop: {
                        userMaterialsSection.highlight = false
                        materialBrowserBundleModel.addToProject(rootView.draggedBundleMaterial)
                    }

                    Grid {
                        id: grid

                        width: scrollView.width
                        leftPadding: 5
                        rightPadding: 5
                        bottomPadding: 5
                        columns: root.width / root.cellWidth

                        Repeater {
                            id: gridRepeater

                            model: materialBrowserModel
                            delegate: MaterialItem {
                                width: root.cellWidth
                                height: root.cellHeight

                                onShowContextMenu: {
                                    ctxMenu.popupMenu(this, model)
                                }
                            }
                        }
                    }

                    Text {
                        text: qsTr("No match found.");
                        color: StudioTheme.Values.themeTextColor
                        font.pixelSize: StudioTheme.Values.baseFontSize
                        leftPadding: 10
                        visible: materialBrowserModel.isEmpty && !searchBox.isEmpty() && !materialBrowserModel.hasMaterialRoot
                    }

                    Text {
                        text:qsTr("There are no materials in this project.<br>Select '<b>+</b>' to create one.")
                        visible: materialBrowserModel.isEmpty && searchBox.isEmpty()
                        textFormat: Text.RichText
                        color: StudioTheme.Values.themeTextColor
                        font.pixelSize: StudioTheme.Values.mediumFontSize
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        width: root.width
                    }
                }

                Section {
                    id: bundleMaterialsSection

                    width: root.width
                    caption: qsTr("Material Library")
                    addTopPadding: noMatchText.visible
                    visible: materialBrowserBundleModel.matBundleExists

                    Column {
                        Repeater {
                            id: bundleMaterialsSectionRepeater

                            model: materialBrowserBundleModel

                            delegate: Section {
                                width: root.width
                                caption: bundleCategoryName
                                addTopPadding: false
                                sectionBackgroundColor: "transparent"
                                visible: bundleCategoryVisible
                                expanded: bundleCategoryExpanded
                                expandOnClick: false
                                onToggleExpand: bundleCategoryExpanded = !bundleCategoryExpanded
                                onExpand: bundleCategoryExpanded = true
                                onCollapse: bundleCategoryExpanded = false

                                Grid {
                                    width: scrollView.width
                                    leftPadding: 5
                                    rightPadding: 5
                                    bottomPadding: 5
                                    columns: root.width / root.cellWidth

                                    Repeater {
                                        model: bundleCategoryMaterials

                                        delegate: BundleMaterialItem {
                                            width: root.cellWidth
                                            height: root.cellHeight

                                            onShowContextMenu: {
                                                ctxMenuBundle.popupMenu(modelData)
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Text {
                            id: noMatchText
                            text: qsTr("No match found.");
                            color: StudioTheme.Values.themeTextColor
                            font.pixelSize: StudioTheme.Values.baseFontSize
                            leftPadding: 10
                            visible: materialBrowserBundleModel.isEmpty && !searchBox.isEmpty() && !materialBrowserModel.hasMaterialRoot
                        }
                    }
                }
            }
        }
    }
}
