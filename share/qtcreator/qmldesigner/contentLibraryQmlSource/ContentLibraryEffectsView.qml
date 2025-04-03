// Copyright (C) 2023 The Qt Company Ltd.
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
        ContentLibraryEffectContextMenu {
            id: ctxMenu

            onUnimport: (bundleEff) => root.unimport(bundleEff)
        }

        Repeater {
            id: categoryRepeater

            model: ContentLibraryBackend.effectsModel

            delegate: HelperWidgets.Section {
                id: section

                width: root.width
                leftPadding: StudioTheme.Values.sectionPadding
                rightPadding: StudioTheme.Values.sectionPadding
                topPadding: StudioTheme.Values.sectionPadding
                bottomPadding: StudioTheme.Values.sectionPadding

                caption: bundleCategoryName
                visible: bundleCategoryVisible && !ContentLibraryBackend.effectsModel.isEmpty
                expanded: bundleCategoryExpanded
                expandOnClick: false
                category: "ContentLib_Effect"

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
                        model: bundleCategoryItems

                        delegate: ContentLibraryItem {
                            width: root.cellWidth
                            height: root.cellHeight

                            onShowContextMenu: ctxMenu.popupMenu(modelData)
                            onAddToProject: ContentLibraryBackend.effectsModel.addInstance(modelData)
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
                    qsTr("<b>Content Library</b> effects are not supported in Qt5 projects.")
                } else if (!ContentLibraryBackend.rootView.hasQuick3DImport) {
                    qsTr('To use <b>Content Library</b> effects, add the <b>QtQuick3D</b> module and the <b>View3D</b>
                         component in the <b>Components</b> view, or click
                         <a href=\"#add_import\"><span style=\"text-decoration:none;color:%1\">
                         here</span></a>.').arg(StudioTheme.Values.themeInteraction)
                } else if (!ContentLibraryBackend.effectsModel.hasRequiredQuick3DImport) {
                    qsTr("To use <b>Content Library</b>, version 6.4 or later of the QtQuick3D module is required.")
                } else if (!ContentLibraryBackend.rootView.hasMaterialLibrary) {
                    qsTr("<b>Content Library</b> is disabled inside a non-visual component.")
                } else if (!ContentLibraryBackend.effectsModel.bundleExists) {
                    qsTr("No effects available.")
                } else if (!searchBox.isEmpty()) {
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
            visible: ContentLibraryBackend.effectsModel.isEmpty
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
