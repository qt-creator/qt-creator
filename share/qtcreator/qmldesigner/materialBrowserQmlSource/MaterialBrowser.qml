// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import MaterialBrowserBackend

Item {
    id: root
    focus: true

    readonly property real cellWidth: root.thumbnailSize
    readonly property real cellHeight: root.thumbnailSize + 20
    readonly property bool enableUiElements: materialBrowserModel.hasMaterialLibrary
                                             && materialBrowserModel.hasQuick3DImport

    property var currMaterialItem: null
    property var rootView: MaterialBrowserBackend.rootView
    property var materialBrowserModel: MaterialBrowserBackend.materialBrowserModel
    property var materialBrowserTexturesModel: MaterialBrowserBackend.materialBrowserTexturesModel

    property int numColumns: 0
    property real thumbnailSize: 100

    readonly property int minThumbSize: 100
    readonly property int maxThumbSize: 150

    function responsiveResize(width: int, height: int) {
        width -= 2 * StudioTheme.Values.sectionPadding

        let numColumns = Math.floor(width / root.minThumbSize)
        let remainder = width % root.minThumbSize
        let space = (numColumns - 1) * StudioTheme.Values.sectionGridSpacing

        if (remainder < space)
            numColumns -= 1

        if (numColumns < 1)
            return

        let maxItems = Math.max(texturesRepeater.count, materialRepeater.count)

        if (numColumns > maxItems)
            numColumns = maxItems

        let rest = width - (numColumns * root.minThumbSize)
                   - ((numColumns - 1) * StudioTheme.Values.sectionGridSpacing)

        root.thumbnailSize = Math.min(root.minThumbSize + (rest / numColumns),
                                      root.maxThumbSize)
        root.numColumns = numColumns
    }

    onWidthChanged: root.responsiveResize(root.width, root.height)

    // Called also from C++ to close context menu on focus out
    function closeContextMenu() {
        ctxMenu.close()
        ctxMenuTextures.close()
        HelperWidgets.Controller.closeContextMenu()
    }

    // Called from C++ to refresh a preview material after it changes
    function refreshPreview(idx) {
        var item = materialRepeater.itemAt(idx);
        if (item)
            item.refreshPreview()
    }

    // Called from C++
    function clearSearchFilter() {
        searchBox.clear()
    }

    function nextVisibleItem(idx, count, itemModel) {
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

    function visibleItemCount(itemModel) {
        let curIdx = 0
        let count = 0

        for (; curIdx < itemModel.rowCount(); ++curIdx) {
            if (itemModel.isVisible(curIdx))
                ++count
        }

        return count
    }

    function rowIndexOfItem(idx, rowSize, itemModel) {
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

    function selectNextVisibleItem(delta) {
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
                targetIdx = root.nextVisibleItem(materialBrowserModel.selectedIndex,
                                                 delta, materialBrowserModel)
                if (targetIdx >= 0)
                    materialBrowserModel.selectMaterial(targetIdx)
            } else if (texSecFocused) {
                targetIdx = root.nextVisibleItem(materialBrowserTexturesModel.selectedIndex,
                                                 delta, materialBrowserTexturesModel)
                if (targetIdx >= 0) {
                    materialBrowserTexturesModel.selectTexture(targetIdx)
                } else if (!materialBrowserModel.isEmpty && materialsSection.expanded) {
                    targetIdx = root.nextVisibleItem(materialBrowserModel.rowCount(), -1, materialBrowserModel)
                    if (targetIdx >= 0) {
                        if (delta !== -1) {
                            // Try to match column when switching between materials/textures
                            origRowIdx = root.rowIndexOfItem(materialBrowserTexturesModel.selectedIndex,
                                                             -delta, materialBrowserTexturesModel)
                            if (root.visibleItemCount(materialBrowserModel) > origRowIdx) {
                                rowIdx = root.rowIndexOfItem(targetIdx, -delta, materialBrowserModel)
                                if (rowIdx >= origRowIdx) {
                                    newTargetIdx = root.nextVisibleItem(targetIdx,
                                                                        -(rowIdx - origRowIdx),
                                                                        materialBrowserModel)
                                } else {
                                    newTargetIdx = root.nextVisibleItem(targetIdx,
                                                                        -(-delta - origRowIdx + rowIdx),
                                                                        materialBrowserModel)
                                }
                            } else {
                                newTargetIdx = root.nextVisibleItem(materialBrowserModel.rowCount(),
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
                targetIdx = root.nextVisibleItem(materialBrowserModel.selectedIndex,
                                                 delta, materialBrowserModel)
                if (targetIdx >= 0) {
                    materialBrowserModel.selectMaterial(targetIdx)
                } else if (!materialBrowserTexturesModel.isEmpty && texturesSection.expanded) {
                    targetIdx = root.nextVisibleItem(-1, 1, materialBrowserTexturesModel)
                    if (targetIdx >= 0) {
                        if (delta !== 1) {
                            // Try to match column when switching between materials/textures
                            origRowIdx = root.rowIndexOfItem(materialBrowserModel.selectedIndex,
                                                             delta, materialBrowserModel)
                            if (root.visibleItemCount(materialBrowserTexturesModel) > origRowIdx) {
                                if (origRowIdx > 0) {
                                    newTargetIdx = root.nextVisibleItem(targetIdx, origRowIdx,
                                                                        materialBrowserTexturesModel)
                                }
                            } else {
                                newTargetIdx = root.nextVisibleItem(materialBrowserTexturesModel.rowCount(),
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
                targetIdx = root.nextVisibleItem(materialBrowserTexturesModel.selectedIndex,
                                                 delta, materialBrowserTexturesModel)
                if (targetIdx >= 0)
                    materialBrowserTexturesModel.selectTexture(targetIdx)
            }
        }
    }

    Keys.enabled: true
    Keys.onDownPressed: root.selectNextVisibleItem(gridMaterials.columns)
    Keys.onUpPressed: root.selectNextVisibleItem(-gridMaterials.columns)
    Keys.onLeftPressed: root.selectNextVisibleItem(-1)
    Keys.onRightPressed: root.selectNextVisibleItem(1)

    function handleEnterPress() {
        if (searchBox.activeFocus)
            return

        if (!materialBrowserModel.isEmpty && rootView.materialSectionFocused && materialsSection.expanded)
            materialBrowserModel.openMaterialEditor()
        else if (!materialBrowserTexturesModel.isEmpty && !rootView.materialSectionFocused && texturesSection.expanded)
            materialBrowserTexturesModel.openTextureEditor()
    }

    Keys.onEnterPressed: root.handleEnterPress()
    Keys.onReturnPressed: root.handleEnterPress()

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

        y: toolbar.height
        width: parent.width
        height: parent.height - toolbar.height

        acceptedButtons: Qt.RightButton

        onClicked: (mouse) => {
            if (!root.enableUiElements)
                return

            var matsSecBottom = mapFromItem(materialsSection, 0, materialsSection.y).y
                                + materialsSection.height

            if (mouse.y < matsSecBottom)
                ctxMenu.popupMenu()
            else
                ctxMenuTextures.popupMenu()
        }
    }

    function ensureVisible(yPos, itemHeight) {
        let currentY = contentYBehavior.targetValue && scrollViewAnim.running
            ? contentYBehavior.targetValue : scrollView.contentY

        if (currentY > yPos) {
            if (yPos < itemHeight)
                scrollView.contentY = 0
            else
                scrollView.contentY = yPos
            return true
        } else {
            let adjustedY = yPos + itemHeight - scrollView.height + 8
            if (currentY < adjustedY) {
                if (scrollView.contentHeight - scrollView.height < adjustedY )
                    scrollView.contentY = scrollView.contentHeight - scrollView.height
                else
                    scrollView.contentY = adjustedY
                return true
            }
        }

        return false
    }

    function ensureSelectedVisible() {
        if (rootView.materialSectionFocused && materialsSection.expanded && root.currMaterialItem
                && materialBrowserModel.isVisible(materialBrowserModel.selectedIndex)) {
            return root.ensureVisible(root.currMaterialItem.mapToItem(scrollView.contentItem, 0, 0).y,
                                      root.currMaterialItem.height)
        } else if (!rootView.materialSectionFocused && texturesSection.expanded) {
            let currItem = texturesRepeater.itemAt(materialBrowserTexturesModel.selectedIndex)
            if (currItem && materialBrowserTexturesModel.isVisible(materialBrowserTexturesModel.selectedIndex))
                return root.ensureVisible(currItem.mapToItem(scrollView.contentItem, 0, 0).y,
                                          currItem.height)
        } else {
            return root.ensureVisible(0, 90)
        }
    }

    Timer {
        id: ensureTimer
        interval: 20
        repeat: true
        triggeredOnStart: true

        onTriggered: {
            // Redo until ensuring didn't change things
            if (!root.ensureSelectedVisible()) {
                ensureTimer.stop()
                ensureTimer.interval = 20
                ensureTimer.triggeredOnStart = true
            }
        }
    }

    function startDelayedEnsureTimer(delay) {
        // Ensuring visibility immediately in some cases like before new search results are rendered
        // causes mapToItem return incorrect values, leading to undesirable flicker,
        // so delay ensuring visibility a bit.
        ensureTimer.interval = delay
        ensureTimer.triggeredOnStart = false
        ensureTimer.restart()
    }

    Connections {
        target: materialBrowserModel

        function onSelectedIndexChanged() {
            // commit rename upon changing selection
            if (root.currMaterialItem)
                root.currMaterialItem.forceFinishEditing();

            root.currMaterialItem = materialRepeater.itemAt(materialBrowserModel.selectedIndex);

            ensureTimer.start()
        }

        function onIsEmptyChanged() {
            ensureTimer.start()
        }
    }

    Connections {
        target: materialBrowserTexturesModel

        function onSelectedIndexChanged() {
            ensureTimer.start()
        }

        function onIsEmptyChanged() {
            ensureTimer.start()
        }
    }

    Connections {
        target: rootView

        function onMaterialSectionFocusedChanged() {
            ensureTimer.start()
        }
    }

    MaterialBrowserContextMenu {
        id: ctxMenu
        onClosed: {
            if (restoreFocusOnClose)
                scrollView.forceActiveFocus()
        }
    }

    TextureBrowserContextMenu {
        id: ctxMenuTextures
        onClosed: {
            scrollView.forceActiveFocus()
        }
    }

    component DoubleButton: Rectangle {
        id: doubleButton

        signal clicked()

        property alias icon: iconLabel.text
        property alias tooltip: mouseArea.tooltip

        property StudioTheme.ControlStyle style: StudioTheme.Values.viewBarButtonStyle

        width: doubleButton.style.squareControlSize.width * 2
        height: doubleButton.style.squareControlSize.height
        radius: StudioTheme.Values.smallRadius

        Row {
            id: contentRow
            spacing: 0

            Text {
                id: iconLabel
                width: doubleButton.style.squareControlSize.width
                height: doubleButton.height
                text: StudioTheme.Constants.material_medium
                font.family: StudioTheme.Constants.iconFont.family
                font.pixelSize: doubleButton.style.baseIconFontSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            Text {
                id: plusLabel
                width: doubleButton.style.squareControlSize.width
                height: doubleButton.height
                text: StudioTheme.Constants.add_medium
                font.family: StudioTheme.Constants.iconFont.family
                font.pixelSize: doubleButton.style.baseIconFontSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        HelperWidgets.ToolTipArea {
            id: mouseArea
            anchors.fill: parent
            onClicked: doubleButton.clicked()
        }

        states: [
            State {
                name: "default"
                when: doubleButton.enabled && !mouseArea.containsMouse && !mouseArea.pressed
                PropertyChanges {
                    target: doubleButton
                    color: doubleButton.style.background.idle
                    border.color: doubleButton.style.border.idle
                }
                PropertyChanges {
                    target: iconLabel
                    color: doubleButton.style.icon.idle
                }
                PropertyChanges {
                    target: plusLabel
                    color: doubleButton.style.icon.idle
                }
            },
            State {
                name: "hover"
                when: doubleButton.enabled && mouseArea.containsMouse && !mouseArea.pressed
                PropertyChanges {
                    target: doubleButton
                    color: doubleButton.style.background.hover
                    border.color: doubleButton.style.border.hover
                }
                PropertyChanges {
                    target: iconLabel
                    color: doubleButton.style.icon.hover
                }
                PropertyChanges {
                    target: plusLabel
                    color: doubleButton.style.icon.hover
                }
            },
            State {
                name: "pressed"
                when: doubleButton.enabled && mouseArea.containsMouse && mouseArea.pressed
                PropertyChanges {
                    target: doubleButton
                    color: doubleButton.style.interaction
                    border.color: doubleButton.style.interaction
                }
                PropertyChanges {
                    target: iconLabel
                    color: doubleButton.style.icon.interaction
                }
                PropertyChanges {
                    target: plusLabel
                    color: doubleButton.style.icon.interaction
                }
            },
            State {
                name: "pressedButNotHovered"
                when: doubleButton.enabled && !mouseArea.containsMouse && mouseArea.pressed
                extend: "hover"
            },
            State {
                name: "disable"
                when: !doubleButton.enabled
                PropertyChanges {
                    target: doubleButton
                    color: doubleButton.style.background.disabled
                    border.color: doubleButton.style.border.disabled
                }
                PropertyChanges {
                    target: iconLabel
                    color: doubleButton.style.icon.disabled
                }
                PropertyChanges {
                    target: plusLabel
                    color: doubleButton.style.icon.disabled
                }
            }
        ]
    }

    Column {
        id: col
        anchors.fill: parent
        spacing: 5

        Rectangle {
            id: toolbar
            width: parent.width
            height: StudioTheme.Values.doubleToolbarHeight
            color: StudioTheme.Values.themeToolbarBackground

            Column {
                anchors.fill: parent
                anchors.topMargin: StudioTheme.Values.toolbarVerticalMargin
                anchors.bottomMargin: StudioTheme.Values.toolbarVerticalMargin
                anchors.leftMargin: StudioTheme.Values.toolbarHorizontalMargin
                anchors.rightMargin: StudioTheme.Values.toolbarHorizontalMargin
                spacing: StudioTheme.Values.toolbarColumnSpacing

                StudioControls.SearchBox {
                    id: searchBox
                    width: parent.width
                    style: StudioTheme.Values.searchControlStyle

                    property string previousSearchText: ""
                    property bool materialsExpanded: true
                    property bool texturesExpanded: true

                    onSearchChanged: (searchText) => {
                        if (searchText !== "") {
                            if (previousSearchText === "") {
                                materialsExpanded = materialsSection.expanded
                                texturesExpanded = texturesSection.expanded
                            }
                            materialsSection.expanded = true
                            texturesSection.expanded = true
                        } else if (previousSearchText !== "") {
                            materialsSection.expanded = materialsExpanded
                            texturesSection.expanded = texturesExpanded
                        }
                        previousSearchText = searchText

                        root.startDelayedEnsureTimer(50)

                        rootView.handleSearchFilterChanged(searchText)
                    }
                }

                Row {
                    width: parent.width
                    height: StudioTheme.Values.toolbarHeight
                    spacing: 6

                    DoubleButton {
                        id: addMaterial
                        icon: StudioTheme.Constants.material_medium
                        tooltip: qsTr("Add a Material.")
                        onClicked: materialBrowserModel.addNewMaterial()
                        enabled: root.enableUiElements
                    }

                    DoubleButton {
                        id: addTexture
                        icon: StudioTheme.Constants.textures_medium
                        tooltip: qsTr("Add a Texture.")
                        onClicked: materialBrowserTexturesModel.addNewTexture()
                        enabled: root.enableUiElements
                    }
                }
            }
        }

        Item {
            width: root.width
            height: root.height - toolbar.height
            visible: hint.text !== ""

            Text {
                id: hint
                width: parent.width - 40
                anchors.centerIn: parent

                text: {
                    if (!materialBrowserModel.isQt6Project)
                        qsTr("<b>Material Browser</b> is not supported in Qt5 projects.")
                    else if (!materialBrowserModel.hasQuick3DImport)
                        qsTr("To use <b>Material Browser</b>, first add the QtQuick3D module in the <b>Components</b> view.")
                    else if (!materialBrowserModel.hasMaterialLibrary)
                        qsTr("<b>Material Browser</b> is disabled inside a non-visual component.")
                    else
                        ""
                }

                textFormat: Text.RichText
                color: StudioTheme.Values.themeTextColor
                font.pixelSize: StudioTheme.Values.mediumFontSize
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
        }

        HelperWidgets.ScrollView {
            id: scrollView

            width: root.width
            height: root.height - toolbar.height - col.spacing
            clip: true
            visible: root.enableUiElements
            hideHorizontalScrollBar: true
            interactive: !ctxMenu.opened && !ctxMenuTextures.opened && !rootView.isDragging
                         && !HelperWidgets.Controller.contextMenuOpened

            Behavior on contentY {
                id: contentYBehavior
                PropertyAnimation {
                    id: scrollViewAnim
                    easing.type: Easing.InOutQuad
                }
            }

            Column {
                Item {
                    width: root.width
                    height: materialsSection.height

                    HelperWidgets.Section {
                        id: materialsSection

                        width: root.width

                        leftPadding: StudioTheme.Values.sectionPadding
                        rightPadding: StudioTheme.Values.sectionPadding
                        topPadding: StudioTheme.Values.sectionPadding
                        bottomPadding: StudioTheme.Values.sectionPadding

                        caption: qsTr("Materials")
                        dropEnabled: true
                        category: "MaterialBrowser"

                        onDropEnter: (drag) => {
                            drag.accepted = drag.formats[0] === "application/vnd.qtdesignstudio.bundlematerial"
                            materialsSection.highlight = drag.accepted
                        }

                        onDropExit: {
                            materialsSection.highlight = false
                        }

                        onDrop: (drag) => {
                            drag.accept()
                            materialsSection.highlight = false
                            rootView.acceptBundleMaterialDrop()
                        }

                        onExpandedChanged: {
                            if (expanded) {
                                if (root.visibleItemCount(materialBrowserModel) > 0)
                                    rootView.focusMaterialSection(true)
                                if (!searchBox.activeFocus)
                                    scrollView.forceActiveFocus()
                            } else {
                                root.startDelayedEnsureTimer(300) // wait for section collapse animation
                                rootView.focusMaterialSection(false)
                            }
                        }

                        Grid {
                            id: gridMaterials

                            width: scrollView.width - materialsSection.leftPadding
                                   - materialsSection.rightPadding
                            spacing: StudioTheme.Values.sectionGridSpacing
                            columns: root.numColumns

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

                                    onShowContextMenu: ctxMenu.popupMenu(this, model)
                                }

                                onCountChanged: root.responsiveResize(root.width, root.height)
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
                        leftPadding: StudioTheme.Values.sectionPadding
                        rightPadding: StudioTheme.Values.sectionPadding
                        topPadding: StudioTheme.Values.sectionPadding
                        bottomPadding: StudioTheme.Values.sectionPadding

                        caption: qsTr("Textures")
                        category: "MaterialBrowser"

                        dropEnabled: true

                        onDropEnter: (drag) => {
                            let accepted = drag.formats[0] === "application/vnd.qtdesignstudio.bundletexture"
                            if (drag.formats[0] === "application/vnd.qtdesignstudio.assets")
                                accepted = rootView.hasAcceptableAssets(drag.urls)
                            drag.accepted = accepted
                            highlight = drag.accepted
                        }

                        onDropExit: {
                            highlight = false
                        }

                        onDrop: (drag) => {
                            drag.accept()
                            highlight = false
                            if (drag.formats[0] === "application/vnd.qtdesignstudio.bundletexture")
                                rootView.acceptBundleTextureDrop()
                            else if (drag.formats[0] === "application/vnd.qtdesignstudio.assets")
                                rootView.acceptAssetsDrop(drag.urls)
                        }

                        onExpandedChanged: {
                            if (expanded) {
                                if (root.visibleItemCount(materialBrowserTexturesModel) > 0)
                                    rootView.focusMaterialSection(false)
                                if (!searchBox.activeFocus)
                                    scrollView.forceActiveFocus()
                            } else {
                                root.startDelayedEnsureTimer(300) // wait for section collapse animation
                                rootView.focusMaterialSection(true)
                            }
                        }

                        Grid {
                            id: gridTextures

                            width: scrollView.width - texturesSection.leftPadding
                                   - texturesSection.rightPadding
                            spacing: StudioTheme.Values.sectionGridSpacing
                            columns: root.numColumns

                            Repeater {
                                id: texturesRepeater

                                model: materialBrowserTexturesModel
                                delegate: TextureItem {
                                    width: root.cellWidth
                                    height: root.cellHeight

                                    onShowContextMenu: ctxMenuTextures.popupMenu(model)
                                }

                                onCountChanged: root.responsiveResize(root.width, root.height)
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
