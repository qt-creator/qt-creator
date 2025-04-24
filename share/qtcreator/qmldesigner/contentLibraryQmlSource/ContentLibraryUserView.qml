// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
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

    ColumnLayout {
        id: col

        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: toolbar

            width: parent.width
            height: StudioTheme.Values.toolbarHeight
            color: StudioTheme.Values.themeToolbarBackground

            HelperWidgets.AbstractButton {
                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.add_medium
                enabled: (this.hasMaterial ?? false)
                      && hasModelSelection
                      && hasQuick3DImport
                      && hasMaterialLibrary
                tooltip: qsTr("Add a custom bundle folder.")
                onClicked: ContentLibraryBackend.rootView.browseBundleFolder()
                x: 5 // left margin
            }
        }

        HelperWidgets.ScrollView {
            id: scrollView

            Layout.fillWidth: true
            Layout.fillHeight: true

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
                        captionTooltip: section.isCustomCat ? categoryBundlePath : ""
                        dropEnabled: true
                        category: "ContentLib_User"
                        showCloseButton: section.isCustomCat
                        closeButtonToolTip: qsTr("Remove folder")
                        closeButtonIcon: StudioTheme.Constants.deletepermanently_small

                        onCloseButtonClicked: {
                            ContentLibraryBackend.userModel.removeBundleDir(index)
                        }

                        function expandSection() {
                            section.expanded = true
                        }

                        property alias count: repeater.count
                        property bool isCustomCat: !["Textures", "Materials", "3D"].includes(section.caption);

                        onCountChanged: root.assignMaxCount()

                        onDropEnter: (drag) => {
                            let has3DNode = ContentLibraryBackend.rootView
                                         .has3DNode(drag.getDataAsArrayBuffer(drag.formats[0]))

                            let hasTexture = ContentLibraryBackend.rootView
                                         .hasTexture(drag.formats[0], drag.urls)

                            drag.accepted = (categoryTitle === "Textures" && hasTexture)
                                         || (categoryTitle === "Materials" && drag.formats[0] === "application/vnd.qtdesignstudio.material")
                                         || (categoryTitle === "3D" && has3DNode)
                                         || (section.isCustomCat && hasTexture)

                            section.highlight = drag.accepted
                        }

                        onDropExit: {
                            section.highlight = false
                        }

                        onDrop: (drag) => {
                            section.highlight = false
                            drag.accept()
                            section.expandSection()

                            if (categoryTitle === "Textures") {
                                if (drag.formats[0] === "application/vnd.qtdesignstudio.assets")
                                    ContentLibraryBackend.rootView.acceptTexturesDrop(drag.urls)
                                else if (drag.formats[0] === "application/vnd.qtdesignstudio.texture")
                                    ContentLibraryBackend.rootView.acceptTextureDrop(drag.getDataAsString(drag.formats[0]))
                            } else if (categoryTitle === "Materials") {
                                ContentLibraryBackend.rootView.acceptMaterialDrop(drag.getDataAsString(drag.formats[0]))
                            } else if (categoryTitle === "3D") {
                                ContentLibraryBackend.rootView.accept3DDrop(drag.getDataAsArrayBuffer(drag.formats[0]))
                            } else { // custom bundle folder
                                if (drag.formats[0] === "application/vnd.qtdesignstudio.assets")
                                    ContentLibraryBackend.rootView.acceptTexturesDrop(drag.urls, categoryBundlePath)
                                else if (drag.formats[0] === "application/vnd.qtdesignstudio.texture")
                                    ContentLibraryBackend.rootView.acceptTextureDrop(drag.getDataAsString(drag.formats[0]), categoryBundlePath)
                            }
                        }

                        Grid {
                            id: grid

                            width: section.width - section.leftPadding - section.rightPadding
                            spacing: StudioTheme.Values.sectionGridSpacing
                            columns: root.numColumns

                            property int catIdx: index

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
                                            visible: modelData.bundleItemVisible && !infoText.visible

                                            onShowContextMenu: ctxMenuItem.popupMenu(modelData)
                                            onAddToProject: ContentLibraryBackend.userModel.addToProject(modelData)
                                        }
                                    }
                                    DelegateChoice {
                                        roleValue: "UserTextures"
                                        delegate: ContentLibraryTexture {
                                            width: root.cellWidth
                                            height: root.cellWidth // for textures use a square size since there is no name row

                                            onShowContextMenu: ctxMenuTexture.popupMenu(modelData, grid.catIdx > 2)
                                        }
                                    }
                                    DelegateChoice {
                                        roleValue: "User3D"
                                        delegate: ContentLibraryItem {
                                            width: root.cellWidth
                                            height: root.cellHeight
                                            visible: modelData.bundleItemVisible && !infoText.visible

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
                                if (!ContentLibraryBackend.rootView.isQt6Project) {
                                    qsTr("<b>Content Library</b> is not supported in Qt5 projects.")
                                } else if (!ContentLibraryBackend.rootView.hasQuick3DImport
                                           && categoryTitle !== "Textures" && !section.isCustomCat) {
                                    qsTr('To use %1, add the <b>QtQuick3D</b> module and the <b>View3D</b>
                                         component in the <b>Components</b> view, or click
                                         <a href=\"#add_import\"><span style=\"text-decoration:none;color:%2\">
                                         here</span></a>.')
                                    .arg(categoryName)
                                    .arg(StudioTheme.Values.themeInteraction)
                                } else if (!ContentLibraryBackend.rootView.hasMaterialLibrary
                                           && categoryTitle !== "Textures" && !section.isCustomCat) {
                                    qsTr("<b>Content Library</b> is disabled inside a non-visual component.")
                                } else if (categoryEmpty) {
                                    qsTr("There are no items in this category.")
                                } else {
                                    ""
                                }
                            }
                            textFormat: Text.RichText
                            color: StudioTheme.Values.themeTextColor
                            font.pixelSize: StudioTheme.Values.mediumFontSize
                            padding: 10
                            visible: infoText.text !== ""
                            horizontalAlignment: Text.AlignHCenter
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
            }
        }
    }
}
