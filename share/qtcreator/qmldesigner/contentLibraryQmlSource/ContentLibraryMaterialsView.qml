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
        ContentLibraryItemContextMenu {
            id: ctxMenu

            onApplyToSelected: (add) => root.materialsModel.applyToSelected(ctxMenu.targetItem, add)

            onUnimport: root.unimport(ctxMenu.targetItem)
            onAddToProject: root.materialsModel.addToProject(ctxMenu.targetItem)
        }

        Repeater {
            id: categoryRepeater

            model: root.materialsModel

            delegate: HelperWidgets.Section {
                id: section

                width: root.width
                leftPadding: StudioTheme.Values.sectionPadding
                rightPadding: StudioTheme.Values.sectionPadding
                topPadding: StudioTheme.Values.sectionPadding
                bottomPadding: StudioTheme.Values.sectionPadding

                caption: bundleCategoryName
                visible: bundleCategoryVisible && !root.materialsModel.isEmpty
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
                            onAddToProject: root.materialsModel.addToProject(modelData)
                        }

                        onCountChanged: root.assignMaxCount()
                    }
                }
            }
        }

        Text {
            id: infoText

            text: {
                if (!ContentLibraryBackend.rootView.isQt6Project) {
                    qsTr("<b>Content Library</b> materials are not supported in Qt5 projects.")
                } else if (ContentLibraryBackend.rootView.isMcuProject) {
                    qsTr("<b>Content Library</b> materials are not supported in MCU projects.")
                } else if (!ContentLibraryBackend.rootView.hasQuick3DImport) {
                    qsTr('To use <b>Content Library</b> materials, add the <b>QtQuick3D</b> module and the <b>View3D</b>
                         component in the <b>Components</b> view, or click
                         <a href=\"#add_import\"><span style=\"text-decoration:none;color:%1\">
                         here</span></a>.').arg(StudioTheme.Values.themeInteraction)
                } else if (!root.materialsModel.hasRequiredQuick3DImport) {
                    qsTr("To use <b>Content Library</b>, version 6.3 or later of the QtQuick3D module is required.")
                } else if (!ContentLibraryBackend.rootView.hasMaterialLibrary) {
                    qsTr("<b>Content Library</b> is disabled inside a non-visual component.")
                } else if (!root.materialsModel.bundleExists) {
                    qsTr("No materials available. Make sure you have an internet connection.")
                } else if (!searchBox.isEmpty() && root.materialsModel.isEmpty) {
                    qsTr("No match found.")
                } else {
                    ""
                }
            }
            textFormat: Text.RichText
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.baseFontSize
            topPadding: 10
            leftPadding: 10
            rightPadding: 10
            wrapMode: Text.WordWrap
            width: root.width

            onLinkActivated: ContentLibraryBackend.rootView.addQtQuick3D()

            HoverHandler {
                enabled: infoText.hoveredLink
                cursorShape: Qt.PointingHandCursor
            }
        }
    }
}
