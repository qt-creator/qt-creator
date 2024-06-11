// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import Qt.labs.qmlmodels
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import ContentLibraryBackend

HelperWidgets.ScrollView {
    id: root

    clip: true
    interactive: !ctxMenuItem.opened && !ctxMenuTexture.opened
                 && !ContentLibraryBackend.rootView.isDragging && !HelperWidgets.Controller.contextMenuOpened

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
    signal removeFromContentLib(var bundleItem);

    function closeContextMenu() {
        ctxMenuItem.close()
        ctxMenuTexture.close()
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
            id: ctxMenuItem

            enableRemove: true

            onApplyToSelected: (add) => ContentLibraryBackend.userModel.applyToSelected(ctxMenuItem.targetItem, add)

            onUnimport: root.unimport(ctxMenuItem.targetItem)
            onAddToProject: ContentLibraryBackend.userModel.addToProject(ctxMenuItem.targetItem)
            onRemoveFromContentLib: root.removeFromContentLib(ctxMenuItem.targetItem)
        }

        ContentLibraryTextureContextMenu {
            id: ctxMenuTexture

            enableRemove: true
            hasSceneEnv: ContentLibraryBackend.texturesModel.hasSceneEnv
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
                visible: categoryVisible && infoText.text === ""
                category: "ContentLib_User"

                function expandSection() {
                    section.expanded = true
                }

                property alias count: repeater.count

                onCountChanged: root.assignMaxCount()

                Grid {
                    width: section.width - section.leftPadding - section.rightPadding
                    spacing: StudioTheme.Values.sectionGridSpacing
                    columns: root.numColumns

                    Repeater {
                        id: repeater
                        model: categoryItems

                        delegate: DelegateChooser {
                            role: "itemType"

                            DelegateChoice {
                                roleValue: "material"
                                ContentLibraryItem {
                                    width: root.cellWidth
                                    height: root.cellHeight

                                    onShowContextMenu: ctxMenuItem.popupMenu(modelData)
                                    onAddToProject: ContentLibraryBackend.userModel.addToProject(modelData)
                                }
                            }
                            DelegateChoice {
                                roleValue: "texture"
                                delegate: ContentLibraryTexture {
                                    width: root.cellWidth
                                    height: root.cellWidth // for textures use a square size since there is no name row

                                    onShowContextMenu: ctxMenuTexture.popupMenu(modelData)
                                }
                            }
                            DelegateChoice {
                                roleValue: "3d"
                                delegate: ContentLibraryItem {
                                    width: root.cellWidth
                                    height: root.cellHeight

                                    onShowContextMenu: ctxMenuItem.popupMenu(modelData)
                                }
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
                    visible: infoText.text === "" && !searchBox.isEmpty() && categoryNoMatch
                }
            }
        }

        Text {
            id: infoText
            text: {
                if (!ContentLibraryBackend.rootView.isQt6Project)
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
            visible: infoText.text !== ""
        }
    }
}
