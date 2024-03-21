// Copyright (C) 2024 The Qt Company Ltd.
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

    required property var searchBox

    signal unimport(var bundleItem);

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

            hasModelSelection: ContentLibraryBackend.userModel.hasModelSelection
            importerRunning: ContentLibraryBackend.userModel.importerRunning

            onApplyToSelected: (add) => ContentLibraryBackend.userModel.applyToSelected(ctxMenu.targetMaterial, add)

            onUnimport: root.unimport(ctxMenu.targetMaterial)
            onAddToProject: ContentLibraryBackend.userModel.addToProject(ctxMenu.targetMaterial)
        }

        Repeater {
            id: categoryRepeater

            model: ContentLibraryBackend.userModel

            delegate: HelperWidgets.Section {
                id: section

                width: root.width
                leftPadding: StudioTheme.Values.sectionPadding
                rightPadding: StudioTheme.Values.sectionPadding
                topPadding: StudioTheme.Values.sectionPadding
                bottomPadding: StudioTheme.Values.sectionPadding

                caption: categoryName
                visible: categoryVisible
                expanded: categoryExpanded
                expandOnClick: false
                category: "ContentLib_User"

                onToggleExpand: categoryExpanded = !categoryExpanded
                onExpand: categoryExpanded = true
                onCollapse: categoryExpanded = false

                function expandSection() {
                    categoryExpanded = true
                }

                property alias count: repeater.count

                onCountChanged: root.assignMaxCount()

                property int numVisibleItem: 1 // initially, the tab is invisible so this will be 0

                Grid {
                    width: section.width - section.leftPadding - section.rightPadding
                    spacing: StudioTheme.Values.sectionGridSpacing
                    columns: root.numColumns

                    Repeater {
                        id: repeater
                        model: categoryItems

                        delegate: ContentLibraryMaterial {
                            width: root.cellWidth
                            height: root.cellHeight

                            importerRunning: ContentLibraryBackend.userModel.importerRunning

                            onShowContextMenu: ctxMenu.popupMenu(modelData)
                            onAddToProject: ContentLibraryBackend.userModel.addToProject(modelData)

                            onVisibleChanged: {
                                section.numVisibleItem += visible ? 1 : -1
                            }
                        }

                        onCountChanged: root.assignMaxCount()
                    }
                }

                Text {
                    text: qsTr("No match found.");
                    color: StudioTheme.Values.themeTextColor
                    font.pixelSize: StudioTheme.Values.baseFontSize
                    leftPadding: 10
                    visible: !searchBox.isEmpty() && section.numVisibleItem === 0
                }
            }
        }

        Text {
            id: infoText
            text: {
                if (!ContentLibraryBackend.effectsModel.bundleExists)
                    qsTr("User bundle couldn't be found.")
                else if (!ContentLibraryBackend.rootView.isQt6Project)
                    qsTr("<b>Content Library</b> is not supported in Qt5 projects.")
                else if (!ContentLibraryBackend.rootView.hasQuick3DImport)
                    qsTr("To use <b>Content Library</b>, first add the QtQuick3D module in the <b>Components</b> view.")
                else if (!ContentLibraryBackend.rootView.hasMaterialLibrary)
                    qsTr("<b>Content Library</b> is disabled inside a non-visual component.")
                else
                    ""
            }
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.baseFontSize
            topPadding: 10
            leftPadding: 10
            visible: ContentLibraryBackend.effectsModel.isEmpty
        }
    }
}
