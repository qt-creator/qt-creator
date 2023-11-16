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

    property int cellWidth: 100
    property int cellHeight: 100
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

    required property var searchBox
    required property var model
    required property string sectionCategory

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
        ContentLibraryTextureContextMenu {
            id: ctxMenu

            hasSceneEnv: root.model.hasSceneEnv
        }

        Repeater {
            id: categoryRepeater

            model: root.model

            delegate: HelperWidgets.Section {
                id: section

                width: root.width
                leftPadding: StudioTheme.Values.sectionPadding
                rightPadding: StudioTheme.Values.sectionPadding
                topPadding: StudioTheme.Values.sectionPadding
                bottomPadding: StudioTheme.Values.sectionPadding

                caption: bundleCategoryName
                visible: bundleCategoryVisible && !root.model.isEmpty
                expanded: bundleCategoryExpanded
                expandOnClick: false
                category: root.sectionCategory

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
                        model: bundleCategoryTextures

                        delegate: ContentLibraryTexture {
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
                if (!root.model.texBundleExists)
                    qsTr("No textures available. Make sure you have internet connection.")
                else if (!searchBox.isEmpty())
                    qsTr("No match found.")
                else
                    ""
            }
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.baseFontSize
            topPadding: 10
            leftPadding: 10
            visible: root.model.isEmpty
        }
    }
}
