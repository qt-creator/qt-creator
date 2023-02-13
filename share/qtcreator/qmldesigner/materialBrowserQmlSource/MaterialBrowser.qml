// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0 as HelperWidgets
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

    function nextVisibleItem(idx, count, itemModel)
    {
        if (count === 0)
            return idx

        let pos = 0
        let newIdx = idx
        let direction = 1
        if (count < 0)
            direction = -1

        while (pos !== count) {
            newIdx += direction
            if (newIdx < 0 || newIdx >= itemModel.rowCount())
                return -1
            if (itemModel.isVisible(newIdx))
                pos += direction
        }

        return newIdx
    }

    function visibleItemCount(itemModel)
    {
        let curIdx = 0
        let count = 0

        for (; curIdx < itemModel.rowCount(); ++curIdx) {
            if (itemModel.isVisible(curIdx))
                ++count
        }

        return count
    }

    function rowIndexOfItem(idx, rowSize, itemModel)
    {
        if (rowSize === 1)
            return 1

        let curIdx = 0
        let count = -1

        while (curIdx <= idx) {
            if (curIdx >= itemModel.rowCount())
                break
            if (itemModel.isVisible(curIdx))
                ++count
            ++curIdx
        }

        return count % rowSize
    }

    function selectNextVisibleItem(delta)
    {
        if (searchBox.activeFocus)
            return

        let targetIdx = -1
        let newTargetIdx = -1
        let origRowIdx = -1
        let rowIdx = -1
        let matSecFocused = rootView.materialSectionFocused && materialsSection.expanded
        let texSecFocused = !rootView.materialSectionFocused && texturesSection.expanded

        if (delta < 0) {
            if (matSecFocused) {
                targetIdx = nextVisibleItem(materialBrowserModel.selectedIndex,
                                            delta, materialBrowserModel)
                if (targetIdx >= 0)
                    materialBrowserModel.selectMaterial(targetIdx)
            } else if (texSecFocused) {
                targetIdx = nextVisibleItem(materialBrowserTexturesModel.selectedIndex,
                                            delta, materialBrowserTexturesModel)
                if (targetIdx >= 0) {
                    materialBrowserTexturesModel.selectTexture(targetIdx)
                } else if (!materialBrowserModel.isEmpty && materialsSection.expanded) {
                    targetIdx = nextVisibleItem(materialBrowserModel.rowCount(), -1, materialBrowserModel)
                    if (targetIdx >= 0) {
                        if (delta !== -1) {
                            // Try to match column when switching between materials/textures
                            origRowIdx  = rowIndexOfItem(materialBrowserTexturesModel.selectedIndex,
                                                         -delta, materialBrowserTexturesModel)
                            if (visibleItemCount(materialBrowserModel) > origRowIdx) {
                                rowIdx = rowIndexOfItem(targetIdx, -delta, materialBrowserModel)
                                if (rowIdx >= origRowIdx) {
                                    newTargetIdx = nextVisibleItem(targetIdx,
                                                                   -(rowIdx - origRowIdx),
                                                                   materialBrowserModel)
                                } else {
                                    newTargetIdx = nextVisibleItem(targetIdx,
                                                                   -(-delta - origRowIdx + rowIdx),
                                                                   materialBrowserModel)
                                }
                            } else {
                                newTargetIdx = nextVisibleItem(materialBrowserModel.rowCount(),
                                                               -1, materialBrowserModel)
                            }
                            if (newTargetIdx >= 0)
                                targetIdx = newTargetIdx
                        }
                        materialBrowserModel.selectMaterial(targetIdx)
                        rootView.focusMaterialSection(true)
                    }
                }
            }
        } else if (delta > 0) {
            if (matSecFocused) {
                targetIdx = nextVisibleItem(materialBrowserModel.selectedIndex,
                                            delta, materialBrowserModel)
                if (targetIdx >= 0) {
                    materialBrowserModel.selectMaterial(targetIdx)
                } else if (!materialBrowserTexturesModel.isEmpty && texturesSection.expanded) {
                    targetIdx = nextVisibleItem(-1, 1, materialBrowserTexturesModel)
                    if (targetIdx >= 0) {
                        if (delta !== 1) {
                            // Try to match column when switching between materials/textures
                            origRowIdx  = rowIndexOfItem(materialBrowserModel.selectedIndex,
                                                         delta, materialBrowserModel)
                            if (visibleItemCount(materialBrowserTexturesModel) > origRowIdx) {
                                if (origRowIdx > 0) {
                                    newTargetIdx = nextVisibleItem(targetIdx, origRowIdx,
                                                                   materialBrowserTexturesModel)
                                }
                            } else {
                                newTargetIdx = nextVisibleItem(materialBrowserTexturesModel.rowCount(),
                                                            -1, materialBrowserTexturesModel)
                            }
                            if (newTargetIdx >= 0)
                                targetIdx = newTargetIdx
                        }
                        materialBrowserTexturesModel.selectTexture(targetIdx)
                        rootView.focusMaterialSection(false)
                    }
                }
            } else if (texSecFocused) {
                targetIdx = nextVisibleItem(materialBrowserTexturesModel.selectedIndex,
                                            delta, materialBrowserTexturesModel)
                if (targetIdx >= 0)
                    materialBrowserTexturesModel.selectTexture(targetIdx)
            }
        }
    }

    Keys.enabled: true
    Keys.onDownPressed: {
        selectNextVisibleItem(gridMaterials.columns)
    }

    Keys.onUpPressed: {
        selectNextVisibleItem(-gridMaterials.columns)
    }

    Keys.onLeftPressed: {
        selectNextVisibleItem(-1)
    }

    Keys.onRightPressed: {
        selectNextVisibleItem(1)
    }

    function handleEnterPress()
    {
        if (searchBox.activeFocus)
            return

        if (!materialBrowserModel.isEmpty && rootView.materialSectionFocused && materialsSection.expanded)
            materialBrowserModel.openMaterialEditor()
        else if (!materialBrowserTexturesModel.isEmpty && !rootView.materialSectionFocused && texturesSection.expanded)
            materialBrowserTexturesModel.openTextureEditor()
    }

    Keys.onEnterPressed: {
        handleEnterPress()
    }

    Keys.onReturnPressed: {
        handleEnterPress()
    }

    MouseArea {
        id: focusGrabber
        y: searchBox.height
        width: parent.width
        height: parent.height - searchBox.height
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onPressed: (mouse) => {
            forceActiveFocus() // Steal focus from name edit
            mouse.accepted = false
        }
        z: 1
    }

    MouseArea {
        id: rootMouseArea

        y: topContent.height
        width: parent.width
        height: parent.height - topContent.height

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

    function ensureVisible(yPos, itemHeight)
    {
        if (yPos < 0) {
            let adjustedY = scrollView.contentY + yPos
            if (adjustedY < itemHeight)
                scrollView.contentY = 0
            else
                scrollView.contentY = adjustedY
        } else if (yPos + itemHeight > scrollView.height) {
            let adjustedY = scrollView.contentY + yPos + itemHeight - scrollView.height + 8
            if (scrollView.contentHeight - adjustedY - scrollView.height < itemHeight)
                scrollView.contentY = scrollView.contentHeight - scrollView.height
            else
                scrollView.contentY = adjustedY
        }
    }

    Connections {
        target: materialBrowserModel

        function onSelectedIndexChanged()
        {
            // commit rename upon changing selection
            if (root.currMaterialItem)
                root.currMaterialItem.commitRename();

            root.currMaterialItem = materialRepeater.itemAt(materialBrowserModel.selectedIndex);

            if (materialsSection.expanded) {
                ensureVisible(root.currMaterialItem.mapToItem(scrollView, 0, 0).y,
                              root.currMaterialItem.height)
            }
        }
    }

    Connections {
        target: materialBrowserTexturesModel

        function onSelectedIndexChanged()
        {
            if (texturesSection.expanded) {
                let currItem = texturesRepeater.itemAt(materialBrowserTexturesModel.selectedIndex)
                ensureVisible(currItem.mapToItem(scrollView, 0, 0).y, currItem.height)
            }
        }
    }

    Connections {
        target: rootView

        function onMaterialSectionFocusedChanged()
        {
            if (rootView.materialSectionFocused && materialsSection.expanded) {
                ensureVisible(root.currMaterialItem.mapToItem(scrollView, 0, 0).y,
                              root.currMaterialItem.height)
            } else if (!rootView.materialSectionFocused && texturesSection.expanded) {
                let currItem = texturesRepeater.itemAt(materialBrowserTexturesModel.selectedIndex)
                ensureVisible(currItem.mapToItem(scrollView, 0, 0).y, currItem.height)
            }
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
        spacing: 5

        Rectangle {
            id: topContent
            width: parent.width
            height: StudioTheme.Values.doubleToolbarHeight
            color: StudioTheme.Values.themeToolbarBackground

            Column {
                anchors.fill: parent
                anchors.topMargin: 6
                anchors.bottomMargin: 6
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 12

                StudioControls.SearchBox {
                    id: searchBox
                    width: parent.width
                    style: StudioTheme.Values.searchControlStyle

                    onSearchChanged: (searchText) => {
                        rootView.handleSearchFilterChanged(searchText)
                    }
                }

                Row {
                    width: parent.width
                    height: StudioTheme.Values.toolbarHeight
                    spacing: 6

                    HelperWidgets.AbstractButton {
                        id: addMaterial
                        style: StudioTheme.Values.viewBarButtonStyle
                        buttonIcon: StudioTheme.Constants.material_medium
                        tooltip: qsTr("Add a Material.")
                        onClicked: materialBrowserModel.addNewMaterial()
                        enabled: root.enableUiElements
                    }

                    HelperWidgets.AbstractButton {
                        id: addTexture
                        style: StudioTheme.Values.viewBarButtonStyle
                        buttonIcon: StudioTheme.Constants.textures_medium
                        tooltip: qsTr("Add a Texture.")
                        onClicked: materialBrowserTexturesModel.addNewTexture()
                        enabled: root.enableUiElements
                    }
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

        HelperWidgets.ScrollView {
            id: scrollView

            width: root.width
            height: root.height - topContent.height
            clip: true
            visible: root.enableUiElements
            interactive: !ctxMenu.opened && !ctxMenuTextures.opened && !rootView.isDragging

            Behavior on contentY {
                PropertyAnimation { easing.type: Easing.InOutQuad }
            }

            Column {
                Item {
                    width: root.width
                    height: materialsSection.height

                    HelperWidgets.Section {
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
                }

                Item {
                    width: root.width
                    height: texturesSection.height

                    HelperWidgets.Section {
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
