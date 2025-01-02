// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import Qt.labs.qmlmodels
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import ContentLibraryBackend

Item {
    id: root

    property alias adsFocus: scrollView.adsFocus

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

    ContentLibraryItemContextMenu {
        id: ctxMenuItem

        showRemoveAction: true
        showImportAction: true

        onApplyToSelected: (add) => ContentLibraryBackend.userModel.applyToSelected(ctxMenuItem.targetItem, add)

        onUnimport: root.unimport(ctxMenuItem.targetItem)
        onAddToProject: ContentLibraryBackend.userModel.addToProject(ctxMenuItem.targetItem)
        onRemoveFromContentLib: root.removeFromContentLib(ctxMenuItem.targetItem)
        onImportBundle: ContentLibraryBackend.rootView.importBundle();
    }

    ContentLibraryTextureContextMenu {
        id: ctxMenuTexture

        showRemoveAction: true
        hasSceneEnv: ContentLibraryBackend.texturesModel.hasSceneEnv
    }

    MouseArea {
        id: rootMouseArea

        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        enabled: ContentLibraryBackend.rootView.isQt6Project
              && ContentLibraryBackend.rootView.hasQuick3DImport
              && ContentLibraryBackend.rootView.hasMaterialLibrary

        onClicked: (mouse) => {
            ctxMenuItem.popupMenu()
        }
    }

    HelperWidgets.ScrollView {
        id: scrollView
        anchors.fill: parent

        clip: true
        interactive: !ctxMenuItem.opened && !ctxMenuTexture.opened
                     && !ContentLibraryBackend.rootView.isDragging && !HelperWidgets.Controller.contextMenuOpened
        hideHorizontalScrollBar: true

        Column {
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

                    caption: categoryTitle
                    dropEnabled: true
                    category: "ContentLib_User"

                    function expandSection() {
                        section.expanded = true
                    }

                    property alias count: repeater.count

                    onCountChanged: root.assignMaxCount()

                    onDropEnter: (drag) => {
                        drag.accepted = (categoryTitle === "Textures" && drag.formats[0] === "application/vnd.qtdesignstudio.assets")
                                     || (categoryTitle === "Materials" && drag.formats[0] === "application/vnd.qtdesignstudio.material")

                        section.highlight = drag.accepted
                    }

                    onDropExit: {
                        section.highlight = false
                    }

                    onDrop: (drag) => {
                        section.highlight = false
                        drag.accept()
                        section.expandSection()

                        if (categoryTitle === "Textures")
                            ContentLibraryBackend.rootView.acceptTexturesDrop(drag.urls)
                        else if (categoryTitle === "Materials")
                            ContentLibraryBackend.rootView.acceptMaterialDrop(drag.getDataAsString(drag.formats[0]))
                    }

                    Grid {
                        width: section.width - section.leftPadding - section.rightPadding
                        spacing: StudioTheme.Values.sectionGridSpacing
                        columns: root.numColumns

                        Repeater {
                            id: repeater
                            model: categoryItems

                            delegate: DelegateChooser {
                                role: "bundleId"

                                DelegateChoice {
                                    roleValue: "UserMaterials"
                                    ContentLibraryItem {
                                        width: root.cellWidth
                                        height: root.cellHeight
                                        visible: !infoText.visible

                                        onShowContextMenu: ctxMenuItem.popupMenu(modelData)
                                        onAddToProject: ContentLibraryBackend.userModel.addToProject(modelData)
                                    }
                                }
                                DelegateChoice {
                                    roleValue: "UserTextures"
                                    delegate: ContentLibraryTexture {
                                        width: root.cellWidth
                                        height: root.cellWidth // for textures use a square size since there is no name row

                                        onShowContextMenu: ctxMenuTexture.popupMenu(modelData)
                                    }
                                }
                                DelegateChoice {
                                    roleValue: "User3D"
                                    delegate: ContentLibraryItem {
                                        width: root.cellWidth
                                        height: root.cellHeight
                                        visible: !infoText.visible

                                        onShowContextMenu: ctxMenuItem.popupMenu(modelData)
                                        onAddToProject: ContentLibraryBackend.userModel.addToProject(modelData)
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

                    Text {
                        id: infoText

                        text: {
                            let categoryName = (categoryTitle === "3D") ? categoryTitle + " assets"
                                                                        : categoryTitle.toLowerCase()

                            if (!ContentLibraryBackend.rootView.isQt6Project)
                                qsTr("<b>Content Library</b> is not supported in Qt5 projects.")
                            else if (!ContentLibraryBackend.rootView.hasQuick3DImport && categoryTitle !== "Textures")
                                qsTr("To use " +  categoryName + ", first add the QtQuick3D module in the <b>Components</b> view.")
                            else if (!ContentLibraryBackend.rootView.hasMaterialLibrary && categoryTitle !== "Textures")
                                qsTr("<b>Content Library</b> is disabled inside a non-visual component.")
                            else if (categoryEmpty)
                                qsTr("There are no "+ categoryName + " in the <b>User Assets</b>.")
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
        }
    }
}
