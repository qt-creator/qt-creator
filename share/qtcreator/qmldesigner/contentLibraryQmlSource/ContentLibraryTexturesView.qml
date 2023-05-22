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

    readonly property int cellWidth: 100
    readonly property int cellHeight: 100

    property var currMaterialItem: null
    property var rootItem: null

    required property var searchBox
    required property var model
    required property string sectionCategory

    signal unimport(var bundleMat);

    function closeContextMenu()
    {
        ctxMenu.close()
    }

    function expandVisibleSections()
    {
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
                width: root.width
                caption: bundleCategoryName
                addTopPadding: false
                sectionBackgroundColor: "transparent"
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

                Grid {
                    width: root.width
                    leftPadding: 5
                    rightPadding: 5
                    bottomPadding: 5
                    spacing: 5
                    columns: root.width / root.cellWidth

                    Repeater {
                        model: bundleCategoryTextures

                        delegate: ContentLibraryTexture {
                            width: root.cellWidth
                            height: root.cellHeight

                            onShowContextMenu: ctxMenu.popupMenu(modelData)
                        }
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
