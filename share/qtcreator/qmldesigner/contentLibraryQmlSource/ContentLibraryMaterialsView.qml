// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import ContentLibraryBackend

HelperWidgets.ScrollView {
    id: root

    clip: true
    interactive: !ctxMenu.opened && !ContentLibraryBackend.rootView.isDragging
                 && !HelperWidgets.Controller.contextMenuOpened

    property real cellWidth: 100
    property real cellHeight: 120
    property int numColumns: 4

    property int count: 0
    function assignMaxCount() {
        let c = 0
        for (let i = 0; i < categoryRepeater.count; ++i)
            c = Math.max(c, categoryRepeater.itemAt(i)?.count ?? 0)

        root.count = c
    }

    property var currMaterialItem: null
    property var rootItem: null
    property var materialsModel: ContentLibraryBackend.materialsModel

    required property var searchBox

    signal unimport(var bundleMat)

    function closeContextMenu() {
        ctxMenu.close()
    }

    function expandVisibleSections() {
        for (let i = 0; i < categoryRepeater.count; ++i) {
            let cat = categoryRepeater.itemAt(i)
            if (cat.visible && !cat.expanded)
                cat.expandSection()
        }
    }

    Column {
        ContentLibraryMaterialContextMenu {
            id: ctxMenu

            hasModelSelection: materialsModel.hasModelSelection
            importerRunning: materialsModel.importerRunning

            onUnimport: (bundleMat) => root.unimport(bundleMat)
            onAddToProject: (bundleMat) => materialsModel.addToProject(bundleMat)
        }

        Repeater {
            id: categoryRepeater

            model: materialsModel

            delegate: HelperWidgets.Section {
                id: section

                width: root.width
                leftPadding: StudioTheme.Values.sectionPadding
                rightPadding: StudioTheme.Values.sectionPadding
                topPadding: StudioTheme.Values.sectionPadding
                bottomPadding: StudioTheme.Values.sectionPadding

                caption: bundleCategoryName
                visible: bundleCategoryVisible && !materialsModel.isEmpty
                expanded: bundleCategoryExpanded
                expandOnClick: false
                category: "ContentLib_Mat"

                onToggleExpand: bundleCategoryExpanded = !bundleCategoryExpanded
                onExpand: bundleCategoryExpanded = true
                onCollapse: bundleCategoryExpanded = false

                function expandSection() {
                    bundleCategoryExpanded = true
                }

                property alias count: repeater.count

                onCountChanged: root.assignMaxCount()

                Grid {
                    width: section.width - section.leftPadding - section.rightPadding
                    spacing: StudioTheme.Values.sectionGridSpacing
                    columns: root.numColumns

                    Repeater {
                        id: repeater
                        model: bundleCategoryMaterials

                        delegate: ContentLibraryMaterial {
                            width: root.cellWidth
                            height: root.cellHeight

                            onShowContextMenu: ctxMenu.popupMenu(modelData)
                        }

                        onCountChanged: root.assignMaxCount()
                    }
                }
            }
        }

        Text {
            id: infoText
            text: {
                if (!materialsModel.matBundleExists)
                    qsTr("No materials available. Make sure you have internet connection.")
                else if (!ContentLibraryBackend.rootView.isQt6Project)
                    qsTr("<b>Content Library</b> materials are not supported in Qt5 projects.")
                else if (!ContentLibraryBackend.rootView.hasQuick3DImport)
                    qsTr("To use <b>Content Library</b>, first add the QtQuick3D module in the <b>Components</b> view.")
                else if (!materialsModel.hasRequiredQuick3DImport)
                    qsTr("To use <b>Content Library</b>, version 6.3 or later of the QtQuick3D module is required.")
                else if (!ContentLibraryBackend.rootView.hasMaterialLibrary)
                    qsTr("<b>Content Library</b> is disabled inside a non-visual component.")
                else if (!searchBox.isEmpty())
                    qsTr("No match found.")
                else
                    ""
            }
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.baseFontSize
            topPadding: 10
            leftPadding: 10
            visible: materialsModel.isEmpty
        }
    }
}
