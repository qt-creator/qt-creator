// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

HelperWidgets.ScrollView {
    id: root

    clip: true

    readonly property int cellWidth: 100
    readonly property int cellHeight: 100

    property var currMaterialItem: null
    property var rootItem: null

    required property var searchBox
    required property var model

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
                cat.expanded = true
        }
    }
    Column {
        ContentLibraryTextureContextMenu {
            id: ctxMenu

            // TODO
        }

        Repeater {
            id: categoryRepeater

            model: root.model

            delegate: HelperWidgets.Section {
                width: root.width
                caption: bundleCategoryName
                addTopPadding: false
                sectionBackgroundColor: "transparent"
                visible: bundleCategoryVisible
                expanded: bundleCategoryExpanded
                expandOnClick: false
                onToggleExpand: bundleCategoryExpanded = !bundleCategoryExpanded

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
            id: noMatchText
            text: qsTr("No match found.");
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.baseFontSize
            topPadding: 10
            leftPadding: 10
            visible: root.model.isEmpty && !searchBox.isEmpty() && !root.model.hasMaterialRoot
        }
    }
}
