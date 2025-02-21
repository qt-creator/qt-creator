// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme as StudioTheme
import StudioControls as StudioControls
import AssetsLibraryBackend

T.TreeViewDelegate {
    id: root

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    required property Item assetsView
    required property Item assetsRoot

    property var assetsModel: AssetsLibraryBackend.assetsModel
    property var rootView: AssetsLibraryBackend.rootView

    property bool hasChildWithDropHover: false
    property bool isHighlighted: false
    property bool isDelegateEmpty: false
    readonly property string suffix: model.fileName.substr(-4)
    readonly property bool isFont: root.suffix === ".ttf" || root.suffix === ".otf"
    readonly property bool isEffect: root.suffix === ".qep"
    readonly property bool isImported3d: root.suffix === ".q3d"
    property bool currFileSelected: false
    property int initialDepth: -1
    property bool __isDirectory: assetsModel.isDirectory(model.filePath)
    property int __currentRow: model.index
    property string __itemPath: model.filePath

    readonly property int __fileItemHeight: thumbnailImage.height + 2 * StudioTheme.Values.border
    readonly property int __dirItemHeight: 21

    implicitHeight: root.__isDirectory ? root.__dirItemHeight : root.__fileItemHeight
    implicitWidth: root.assetsView.width

    Component.onCompleted: {
        // the depth of the root path will become available before we get to the actual
        // items we display, so it's safe to set assetsView.rootPathDepth here. All other
        // tree items (below the root) will have the indentation (basically, depth) adjusted.
        if (model.filePath === assetsModel.rootPath()) {
            root.assetsView.rootPathDepth = root.depth
            root.assetsView.rootPathRow = root.__currentRow
        } else if (model.filePath.includes(assetsModel.rootPath())) {
            root.depth -= root.assetsView.rootPathDepth
            root.initialDepth = root.depth
        }

        // expand/collapse folder based on its stored expanded state
        if (root.__isDirectory) {
            // if the folder expand state is not stored yet, stores it as true (expanded)
            root.assetsModel.initializeExpandState(root.__itemPath)

            let expandState = assetsModel.folderExpandState(root.__itemPath)

            if (expandState)
                root.assetsView.expand(root.__currentRow)
            else
                root.assetsView.collapse(root.__currentRow)

            root.isDelegateEmpty = assetsModel.isDelegateEmpty(root.__itemPath)
        }
    }

    // workaround for a bug -- might be fixed by https://codereview.qt-project.org/c/qt/qtdeclarative/+/442721
    onYChanged: {
        if (root.__currentRow === root.assetsView.firstRow) {
            if (root.y > root.assetsView.contentY) {
                let item = root.assetsView.itemAtCell(0, root.assetsView.rootPathRow)
                if (!item)
                    root.assetsView.contentY = root.y
            }
        }
    }

    onDepthChanged: {
        if (root.depth > root.initialDepth && root.initialDepth >= 0)
            root.depth = root.initialDepth
    }

    indicator: Item {
        id: arrowIndicator

        implicitWidth: 10
        implicitHeight: root.implicitHeight
        anchors.left: bg.left
        anchors.leftMargin: 5

        Image {
            id: arrow

            width: 8
            height: 4
            visible: !root.isDelegateEmpty
            source: "image://icons/down-arrow"
            anchors.centerIn: parent
            rotation: root.expanded ? 0 : -90
        }
    }

    background: Rectangle {
        id: bg

        x: root.indentation * (root.depth - 1)
        width: root.implicitWidth - bg.x

        color: {
            if (root.__isDirectory && (root.isHighlighted || root.hasChildWithDropHover))
                return StudioTheme.Values.themeInteraction

            if (!root.__isDirectory && root.assetsView.selectedAssets[root.__itemPath])
                return StudioTheme.Values.themeSectionHeadBackground

            if (mouseArea.containsMouse)
                return StudioTheme.Values.themeSectionHeadBackground

            return root.__isDirectory
                    ? StudioTheme.Values.themeSectionHeadBackground
                    : "transparent"
        }
        border.width: StudioTheme.Values.border
        border.color: root.assetsView.selectedAssets[root.__itemPath] ? StudioTheme.Values.themeInteraction
                                                                      : "transparent"
    }

    contentItem: Text {
        id: assetLabel

        text: assetLabel.__computeText()
        color: StudioTheme.Values.themeTextColor
        font.pixelSize: StudioTheme.Values.baseFontSize
        verticalAlignment: Qt.AlignVCenter
        anchors.left: root.__isDirectory ? arrowIndicator.right : thumbnailImage.right
        anchors.leftMargin: 8

        function __computeText() {
            return root.__isDirectory
                    ? (root.isDelegateEmpty
                       ? model.display.toUpperCase() + qsTr(" (empty)")
                       : model.display.toUpperCase())
                    : model.display
        }
    }

    MouseArea {
        id: mouseArea

        property bool allowTooltip: true

        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onExited: AssetsLibraryBackend.tooltipBackend.hideTooltip()
        onEntered: mouseArea.allowTooltip = true

        onCanceled: {
            AssetsLibraryBackend.tooltipBackend.hideTooltip()
            mouseArea.allowTooltip = true
        }

        onPositionChanged: AssetsLibraryBackend.tooltipBackend.reposition()

        onPressed: (mouse) => {
            mouseArea.forceActiveFocus()
            mouseArea.allowTooltip = false
            AssetsLibraryBackend.tooltipBackend.hideTooltip()

            var ctrlDown = mouse.modifiers & Qt.ControlModifier

            if (mouse.button === Qt.LeftButton) {
                if (root.__isDirectory) {
                    // ensure only one directory can be selected
                    root.assetsView.clearSelectedAssets()
                    root.currFileSelected = true
                } else {
                    if (!root.assetsView.isAssetSelected(root.__itemPath) && !ctrlDown)
                        root.assetsView.clearSelectedAssets()
                    root.currFileSelected = ctrlDown ? !root.assetsView.isAssetSelected(root.__itemPath) : true
                }

                root.assetsView.setAssetSelected(root.__itemPath, root.currFileSelected)

                if (root.currFileSelected) {
                    let selectedPaths = root.assetsView.selectedPathsAsList()
                    AssetsLibraryBackend.rootView.startDragAsset(selectedPaths, mapToGlobal(mouse.x, mouse.y))
                }
           } else {
                if (!root.assetsView.isAssetSelected(root.__itemPath) && !ctrlDown)
                    root.assetsView.clearSelectedAssets()
                root.currFileSelected = root.assetsView.isAssetSelected(root.__itemPath) || !ctrlDown
                root.assetsView.setAssetSelected(root.__itemPath, root.currFileSelected)
           }
        }

        onReleased: (mouse) => {
            mouseArea.allowTooltip = true

            if (root.__isDirectory)
                return

            if (mouse.button === Qt.LeftButton) {
                if (!(mouse.modifiers & Qt.ControlModifier))
                    root.assetsView.selectedAssets = {}
                root.assetsView.selectedAssets[root.__itemPath] = root.currFileSelected
                root.assetsView.selectedAssetsChanged()
            }
        }

        onDoubleClicked: (mouse) => {
            mouseArea.forceActiveFocus()
            mouseArea.allowTooltip = false
            AssetsLibraryBackend.tooltipBackend.hideTooltip()
            if (mouse.button === Qt.LeftButton) {
                if (root.isEffect)
                    AssetsLibraryBackend.rootView.openEffectComposer(filePath)
                else if (root.isImported3d)
                    AssetsLibraryBackend.rootView.editAssetComponent(filePath)
            }
        }

        StudioControls.ToolTip {
            id: assetTooltip
            visible: !root.isFont && mouseArea.containsMouse && !root.assetsView.contextMenu.visible
            text: assetTooltip.__computeText()
            delay: 1000

            function __computeText() {
                let filePath = model.filePath.replace(assetsModel.contentDirPath(), "")
                let fileSize = rootView.assetFileSize(model.filePath)
                let fileExtMatches = model.filePath.match(/\.(.*)$/)
                let fileExt = fileExtMatches ? "(" + fileExtMatches[1] + ")" : ""

                if (root.__isDirectory)
                    return filePath

                if (rootView.assetIsImageOrTexture(model.filePath)) {
                    let size = rootView.imageSize(model.filePath)

                    return filePath + "\n"
                            + size.width + " x " + size.height
                            + "\n" + fileSize
                            + " " + fileExt
                } else if (rootView.assetIsImported3d(model.filePath)) {
                    return filePath + "\n"
                            + fileExt
                } else {
                    return filePath + "\n"
                            + fileSize
                            + " " + fileExt
                }
            }

            function refresh() {
                assetTooltip.text = assetTooltip.__computeText()
            }
        }

        Timer {
            interval: 1000
            running: mouseArea.containsMouse && mouseArea.allowTooltip
            onTriggered: {
                if (root.isFont) {
                    AssetsLibraryBackend.tooltipBackend.name = model.fileName
                    AssetsLibraryBackend.tooltipBackend.path = model.filePath
                    AssetsLibraryBackend.tooltipBackend.showTooltip()
                }
            }
        }

        onClicked: (mouse) => {
            if (mouse.button === Qt.LeftButton) {
                root.__toggleExpandCurrentRow()
                root.assetsView.currentFilePath = root.__itemPath
            } else {
                root.__openContextMenuForCurrentRow()
            }
        }
    }

    function getDirPath() {
        if (root.__isDirectory)
            return model.filePath
        else
            return assetsModel.parentDirPath(model.filePath)
    }

    function __openContextMenuForCurrentRow() {
        let modelIndex = assetsModel.indexForPath(model.filePath)

        function onFolderCreated(path) {
            if (path)
                root.assetsView.addCreatedFolder(path)
        }

        if (root.__isDirectory) {
            var allExpandedState = root.assetsView.computeAllExpandedState()

            root.assetsView.contextMenu.openContextMenuForDir(modelIndex, model.filePath,
                model.fileName, allExpandedState, onFolderCreated)
       } else {
            let parentDirIndex = assetsModel.parentDirIndex(model.filePath)
            let selectedPaths = root.assetsView.selectedPathsAsList()
            root.assetsView.contextMenu.openContextMenuForFile(modelIndex, parentDirIndex,
                                                               selectedPaths, onFolderCreated)
       }
    }

    function __toggleExpandCurrentRow() {
        if (!root.__isDirectory || root.isDelegateEmpty)
            return

        let index = root.assetsView.__modelIndex(root.__currentRow)
        // if the user manually clicked on a directory, then this is definitely not a
        // an automatic request to expand all.
        root.assetsView.requestedExpandAll = false

        if (root.assetsView.isExpanded(root.__currentRow)) {
            root.assetsView.requestedExpandAll = false
            root.assetsView.collapse(root.__currentRow)
        } else {
            root.assetsView.expand(root.__currentRow)
        }

        assetsModel.saveExpandState(root.__itemPath, root.expanded)
    }

    function reloadImage() {
        if (root.__isDirectory)
            return

        thumbnailImage.source = ""
        thumbnailImage.source = thumbnailImage.__computeSource()
    }

    Image {
        id: thumbnailImage
        visible: !root.__isDirectory
        y: StudioTheme.Values.border
        x: bg.x + StudioTheme.Values.border
        width: 48
        height: 48
        cache: false
        sourceSize.width: 48
        sourceSize.height: 48
        asynchronous: true
        fillMode: Image.Pad
        source: thumbnailImage.__computeSource()

        function __computeSource() {
            return root.__isDirectory
                    ? ""
                    : "image://qmldesigner_assets/" + model.filePath
        }

        onStatusChanged: {
            if (thumbnailImage.status === Image.Ready)
                assetTooltip.refresh()
        }
    }

    DropArea {
        id: dropArea

        anchors.fill: parent
        anchors.bottomMargin: -assetsView.rowSpacing

        function updateParentHighlight(highlight) {
            let index = root.assetsView.__modelIndex(root.__currentRow)
            let parentItem = assetsView.__getDelegateParentForIndex(index)
            if (parentItem)
                parentItem.isHighlighted = highlight

            // highlights the root folder canvas area when dragging over child
            if (root.depth === 1 && !root.__isDirectory)
                root.assetsRoot.highlightCanvas = highlight
        }

        onEntered: (drag) => {
            root.assetsRoot.updateDropExtFiles(drag)
            drag.accepted |= drag.formats[0] === "application/vnd.qtdesignstudio.assets"
                          && !root.assetsModel.isSameOrDescendantPath(drag.urls[0], root.__itemPath)

            if (root.__isDirectory)
                root.isHighlighted = drag.accepted
            else
                dropArea.updateParentHighlight(drag.accepted)
        }

        onDropped: (drag) => {
            if (drag.formats[0] === "application/vnd.qtdesignstudio.assets") {
                root.rootView.invokeAssetsDrop(drag.urls, root.getDirPath())
            } else {
                root.rootView.emitExtFilesDrop(root.assetsRoot.dropSimpleExtFiles,
                                               root.assetsRoot.dropComplexExtFiles,
                                               root.getDirPath())
            }

            root.isHighlighted = false
            dropArea.updateParentHighlight(false)
        }

        onExited: {
            root.isHighlighted = false
            dropArea.updateParentHighlight(false)
        }
    }
}
