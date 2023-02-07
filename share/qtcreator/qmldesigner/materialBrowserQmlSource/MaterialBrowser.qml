// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Item {
    id: root

    readonly property int cellWidth: 100
    readonly property int cellHeight: 120
    readonly property bool enableUiElements: materialBrowserModel.hasMaterialLibrary
                                             && materialBrowserModel.hasQuick3DImport

    property var currMaterialItem: null

    // Called also from C++ to close context menu on focus out
    function closeContextMenu()
    {
        ctxMenu.close()
        ctxMenuTextures.close()
    }

    // Called from C++ to refresh a preview material after it changes
    function refreshPreview(idx)
    {
        var item = materialRepeater.itemAt(idx);
        if (item)
            item.refreshPreview();
    }

    // Called from C++
    function clearSearchFilter()
    {
        searchBox.clear();
    }

    Keys.enabled: true
    Keys.onDownPressed: {
        if (rootView.materialSectionFocused) {
            if (!materialBrowserTexturesModel.isEmpty
                && materialBrowserModel.selectedIndex >= materialBrowserModel.rowCount() - gridMaterials.columns) {
                rootView.materialSectionFocused = false
                materialBrowserTexturesModel.selectTexture(materialBrowserModel.selectedIndex % gridMaterials.columns)
            } else {
                let targetIdx = materialBrowserModel.selectedIndex + gridMaterials.columns
                if (targetIdx < materialBrowserModel.rowCount())
                    materialBrowserModel.selectMaterial(targetIdx)
            }
        } else {
            let targetIdx = materialBrowserTexturesModel.selectedIndex + gridTextures.columns
            if (targetIdx < materialBrowserTexturesModel.rowCount())
                materialBrowserTexturesModel.selectTexture(targetIdx)
        }
    }

    Keys.onUpPressed: {
        if (rootView.materialSectionFocused) {
            let targetIdx = materialBrowserModel.selectedIndex - gridMaterials.columns
            if (targetIdx >= 0)
                materialBrowserModel.selectMaterial(targetIdx)
        } else {
            if (!materialBrowserModel.isEmpty && materialBrowserTexturesModel.selectedIndex < gridTextures.columns) {
                rootView.materialSectionFocused = true
                materialBrowserModel.selectMaterial(materialBrowserTexturesModel.selectedIndex % gridTextures.columns)
            } else {
                let targetIdx = materialBrowserTexturesModel.selectedIndex - gridTextures.columns
                if (targetIdx >= 0)
                    materialBrowserTexturesModel.selectTexture(targetIdx)
            }
        }
    }

    Keys.onLeftPressed: {
        if (rootView.materialSectionFocused) {
            if (materialBrowserModel.selectedIndex > 0)
                materialBrowserModel.selectMaterial(materialBrowserModel.selectedIndex - 1)
        } else {
            if (materialBrowserTexturesModel.selectedIndex > 0) {
                materialBrowserTexturesModel.selectTexture(materialBrowserTexturesModel.selectedIndex - 1)
            } else if (materialBrowserModel.rowCount() > 0) {
                rootView.materialSectionFocused = true
                materialBrowserModel.selectMaterial(materialBrowserModel.rowCount() - 1)
            }
        }
    }

    Keys.onRightPressed: {
        if (rootView.materialSectionFocused) {
            if (materialBrowserModel.selectedIndex < materialBrowserModel.rowCount() - 1) {
                materialBrowserModel.selectMaterial(materialBrowserModel.selectedIndex + 1)
            } else if (materialBrowserTexturesModel.rowCount() > 0) {
                rootView.materialSectionFocused = false
                materialBrowserTexturesModel.selectMaterial(0)
            }
        } else {
            if (materialBrowserTexturesModel.selectedIndex < materialBrowserTexturesModel.rowCount() - 1)
                materialBrowserTexturesModel.selectTexture(materialBrowserTexturesModel.selectedIndex + 1)
        }
    }

    MouseArea {
        id: focusGrabber
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onPressed: (mouse) => {
            forceActiveFocus() // Steal focus from name edit
            mouse.accepted = false
        }
        z: 1
    }

    MouseArea {
        id: rootMouseArea

        anchors.fill: parent

        acceptedButtons: Qt.RightButton

        onClicked: (mouse) => {
            if (!root.enableUiElements)
                return;

            var matsSecBottom = mapFromItem(materialsSection, 0, materialsSection.y).y
                                + materialsSection.height;

            if (mouse.y < matsSecBottom)
                ctxMenu.popupMenu()
            else
                ctxMenuTextures.popupMenu()
        }
    }

    Connections {
        target: materialBrowserModel

        function onSelectedIndexChanged() {
            // commit rename upon changing selection
            if (root.currMaterialItem)
                root.currMaterialItem.commitRename();

            root.currMaterialItem = materialRepeater.itemAt(materialBrowserModel.selectedIndex);
        }
    }

    MaterialBrowserContextMenu {
        id: ctxMenu
    }

    TextureBrowserContextMenu {
        id: ctxMenuTextures
    }

    Column {
        id: col
        y: 5
        spacing: 5

        Row {
            width: root.width
            enabled: root.enableUiElements

            StudioControls.SearchBox {
                id: searchBox

                width: root.width

                onSearchChanged: (searchText) => {
                    rootView.handleSearchFilterChanged(searchText)
                }
            }
        }

        Text {
            text: {
                if (!materialBrowserModel.hasQuick3DImport)
                    qsTr("To use <b>Material Browser</b>, first add the QtQuick3D module in the <b>Components</b> view.")
                else if (!materialBrowserModel.hasMaterialLibrary)
                    qsTr("<b>Material Browser</b> is disabled inside a non-visual component.")
                else
                    ""
            }

            textFormat: Text.RichText
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.mediumFontSize
            topPadding: 30
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            width: root.width
            visible: text !== ""
        }

        ScrollView {
            id: scrollView

            width: root.width
            height: root.height - searchBox.height
            clip: true
            visible: root.enableUiElements
            interactive: !ctxMenu.opened && !ctxMenuTextures.opened && !rootView.isDragging

            Column {
                Item {
                    width: root.width
                    height: materialsSection.height

                    Section {
                        id: materialsSection

                        width: root.width
                        caption: qsTr("Materials")
                        dropEnabled: true

                        onDropEnter: (drag) => {
                            drag.accepted = drag.formats[0] === "application/vnd.qtdesignstudio.bundlematerial"
                            materialsSection.highlight = drag.accepted
                        }

                        onDropExit: {
                            materialsSection.highlight = false
                        }

                        onDrop: {
                            materialsSection.highlight = false
                            rootView.acceptBundleMaterialDrop()
                        }

                        Grid {
                            id: gridMaterials

                            width: scrollView.width
                            leftPadding: 5
                            rightPadding: 5
                            bottomPadding: 5
                            columns: root.width / root.cellWidth

                            Repeater {
                                id: materialRepeater

                                model: materialBrowserModel

                                onItemRemoved: (index, item) => {
                                    if (item === root.currMaterialItem)
                                        root.currMaterialItem = null
                                }

                                delegate: MaterialItem {
                                    width: root.cellWidth
                                    height: root.cellHeight

                                    onShowContextMenu: {
                                        ctxMenu.popupMenu(this, model)
                                    }
                                }
                            }
                        }

                        Text {
                            text: qsTr("No match found.");
                            color: StudioTheme.Values.themeTextColor
                            font.pixelSize: StudioTheme.Values.baseFontSize
                            leftPadding: 10
                            visible: materialBrowserModel.isEmpty && !searchBox.isEmpty()
                        }

                        Text {
                            text:qsTr("There are no materials in this project.<br>Select '<b>+</b>' to create one.")
                            visible: materialBrowserModel.isEmpty && searchBox.isEmpty()
                            textFormat: Text.RichText
                            color: StudioTheme.Values.themeTextColor
                            font.pixelSize: StudioTheme.Values.mediumFontSize
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            width: root.width
                        }
                    }

                    IconButton {
                        id: addMaterialButton

                        tooltip: qsTr("Add a material.")

                        anchors.right: parent.right
                        anchors.rightMargin: scrollView.verticalScrollBarVisible ? 10 : 0
                        icon: StudioTheme.Constants.plus
                        normalColor: "transparent"
                        buttonSize: StudioTheme.Values.sectionHeadHeight
                        onClicked: materialBrowserModel.addNewMaterial()
                        enabled: root.enableUiElements
                    }
                }

                Item {
                    width: root.width
                    height: texturesSection.height

                    Section {
                        id: texturesSection

                        width: root.width
                        caption: qsTr("Textures")

                        dropEnabled: true

                        onDropEnter: (drag) => {
                            drag.accepted = drag.formats[0] === "application/vnd.qtdesignstudio.bundletexture"
                            highlight = drag.accepted
                        }

                        onDropExit: {
                            highlight = false
                        }

                        onDrop: {
                            highlight = false
                            rootView.acceptBundleTextureDrop()
                        }

                        Grid {
                            id: gridTextures

                            width: scrollView.width
                            leftPadding: 5
                            rightPadding: 5
                            bottomPadding: 5
                            columns: root.width / root.cellWidth

                            Repeater {
                                id: texturesRepeater

                                model: materialBrowserTexturesModel
                                delegate: TextureItem {
                                    width: root.cellWidth
                                    height: root.cellWidth

                                    onShowContextMenu: {
                                        ctxMenuTextures.popupMenu(model)
                                    }
                                }
                            }
                        }

                        Text {
                            text: qsTr("No match found.");
                            color: StudioTheme.Values.themeTextColor
                            font.pixelSize: StudioTheme.Values.baseFontSize
                            leftPadding: 10
                            visible: materialBrowserTexturesModel.isEmpty && !searchBox.isEmpty()
                        }

                        Text {
                            text:qsTr("There are no textures in this project.")
                            visible: materialBrowserTexturesModel.isEmpty && searchBox.isEmpty()
                            textFormat: Text.RichText
                            color: StudioTheme.Values.themeTextColor
                            font.pixelSize: StudioTheme.Values.mediumFontSize
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            width: root.width
                        }
                    }

                    IconButton {
                        id: addTextureButton

                        tooltip: qsTr("Add a texture.")

                        anchors.right: parent.right
                        anchors.rightMargin: scrollView.verticalScrollBarVisible ? 10 : 0
                        icon: StudioTheme.Constants.plus
                        normalColor: "transparent"
                        buttonSize: StudioTheme.Values.sectionHeadHeight
                        onClicked: materialBrowserTexturesModel.addNewTexture()
                        enabled: root.enableUiElements
                    }
                }

                DropArea {
                    id: masterDropArea

                    property int emptyHeight: scrollView.height - materialsSection.height - texturesSection.height

                    width: root.width
                    height: emptyHeight > 0 ? emptyHeight : 0

                    enabled: true

                    onEntered: (drag) => texturesSection.dropEnter(drag)
                    onDropped: (drag) => texturesSection.drop(drag)
                    onExited: texturesSection.dropExit()
                }
            }
        }
    }
}
