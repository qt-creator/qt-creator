// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import AssetsLibraryBackend

TreeView {
    id: root
    clip: true
    interactive: verticalScrollBar.visible && !root.contextMenu.opened && !rootView.isDragging
    reuseItems: false
    boundsBehavior: Flickable.StopAtBounds
    rowSpacing: 5

    property bool adsFocus: false
    // objectName is used by the dock widget to find this particular ScrollView
    // and set the ads focus on it.
    objectName: "__mainSrollView"

    property var assetsModel: AssetsLibraryBackend.assetsModel
    property var rootView: AssetsLibraryBackend.rootView
    property var tooltipBackend: AssetsLibraryBackend.tooltipBackend

    required property Item assetsRoot
    required property StudioControls.Menu contextMenu
    property alias verticalScrollBar: verticalScrollBar

    property var selectedAssets: ({})
    // the latest file that was clicked, or changed to via Up or Down keys
    property string currentFilePath: ""

    // used to see if the op requested is to expand or to collapse.
    property int lastRowCount: -1
    // we need this to know if we need to expand further, while we're in onRowsChanged()
    property bool requestedExpandAll: true
    // used to compute the visual depth of the items we show to the user.
    property int rootPathDepth: 0
    property int rootPathRow: 0
    // i.e. first child of the root path
    readonly property int firstRow: root.rootPathRow + 1
    readonly property int lastRow: root.rows - 1
    property var __createdDirectories: []

    rowHeightProvider: (row) => {
        if (row <= root.rootPathRow)
            return 0

        return -1
    }

    HoverHandler { id: hoverHandler }

    ScrollBar.vertical: StudioControls.TransientScrollBar {
        id: verticalScrollBar
        style: StudioTheme.Values.viewStyle
        parent: root
        x: root.width - verticalScrollBar.width
        y: 0
        height: root.availableHeight
        orientation: Qt.Vertical

        show: (hoverHandler.hovered || root.adsFocus || verticalScrollBar.inUse)
              && verticalScrollBar.isNeeded
    }

    model: assetsModel

    onRowsChanged: {
        if (root.rows > root.rootPathRow + 1 && !assetsModel.haveFiles ||
            root.rows <= root.rootPathRow + 1 && assetsModel.haveFiles) {
            assetsModel.syncHaveFiles()
        }

        root.updateRows()
    }

    Timer {
        id: updateRowsTimer
        interval: 200
        repeat: false

        onTriggered: {
            root.updateRows()
        }
    }

    Connections {
        target: rootView

        function onDirectoryCreated(path)
        {
            root.__createdDirectories.push(path)

            updateRowsTimer.restart()
        }

        function onDeleteSelectedAssetsRequested()
        {
            let selectedPaths = root.selectedPathsAsList()
            if (!selectedPaths.length)
                return

            let deleted = assetsModel.requestDeleteFiles(selectedPaths)
            if (!deleted) {
                confirmDeleteFiles.files = selectedPaths
                confirmDeleteFiles.open()
            }
        }
    }

    Connections {
        target: assetsModel
        function onDirectoryLoaded(path)
        {
            // updating rows for safety: the rows might have been created before the
            // directory (esp. the root path) has been loaded, so we must make sure all rows are
            // expanded -- otherwise, the tree may not become visible.

            updateRowsTimer.restart()

            let idx = assetsModel.indexForPath(path)
            let row = root.rowAtIndex(idx)
            let column = root.columnAtIndex(idx)

            if (row >= root.rootPathRow && !root.isExpanded(row))
                root.expand(row)
        }

        function onRootPathChanged()
        {
            // when we switch from one project to another, we need to reset the state of the
            // view: make sure we will do an "expand all" (otherwise, the whole tree might
            // be collapsed, and with our visible root not being the actual root of the tree,
            // the entire tree would be invisible)
            root.lastRowCount = -1
            root.requestedExpandAll = true
        }

        function onFileChanged(filePath)
        {
            rootView.invalidateThumbnail(filePath)

            let index = assetsModel.indexForPath(filePath)
            let cell = root.cellAtIndex(index)
            let fileItem = root.itemAtCell(cell)

            if (fileItem)
                fileItem.reloadImage()
        }

    } // Connections

    function addCreatedFolder(path)
    {
        root.__createdDirectories.push(path)
    }

    function selectedPathsAsList()
    {
        return Object.keys(root.selectedAssets)
            .filter(itemPath => root.selectedAssets[itemPath])
    }

    // workaround for a bug -- might be fixed by https://codereview.qt-project.org/c/qt/qtdeclarative/+/442721
    function resetVerticalScrollPosition()
    {
        root.contentY = 0
    }

    function updateRows()
    {
        if (root.rows <= 0)
            return

        while (root.__createdDirectories.length > 0) {
            let dirPath = root.__createdDirectories.pop()
            let index = assetsModel.indexForPath(dirPath)
            let row = root.rowAtIndex(index)

            if (row > 0)
                root.expand(row)
            else if (row === -1 && assetsModel.indexIsValid(index)) {
                // It is possible that this directory, dirPath, was created inside of a parent
                // directory that was not yet expanded in the TreeView. This can happen with the
                // bridge plugin. In such a situation, we don't have a "row" for it yet, so we have
                // to expand its parents, from root to our `index`
                let parents = assetsModel.parentIndices(index);
                parents.reverse().forEach(idx => {
                    let row = root.rowAtIndex(idx)
                    if (row > 0)
                        root.expand(row)
                })
            }
        }

        // we have no way to know beyond doubt here if updateRows() was called due
        // to a request to expand or to collapse rows - but it should be safe to
        // assume that, if we have more rows now than the last time, then it's an expand
        var expanding = (root.rows >= root.lastRowCount)

        if (expanding) {
            if (root.requestedExpandAll)
                root.__doExpandAll()
        } else {
            // on collapsing, set expandAll flag to false.
            root.requestedExpandAll = false;
        }

        root.lastRowCount = root.rows
    }

    function __doExpandAll()
    {
        let expandedAny = false
        for (let nRow = 0; nRow < root.rows; ++nRow) {
            let index = root.__modelIndex(nRow)
            if (assetsModel.isDirectory(index) && !root.isExpanded(nRow)) {
                root.expand(nRow);
                expandedAny = true
            }
        }

        if (!expandedAny)
            Qt.callLater(root.forceLayout)
    }

    function expandAll()
    {
        // In order for __doExpandAll() to be called repeatedly (every time a new node is
        // loaded, and then, expanded), we need to set requestedExpandAll to true.
        root.requestedExpandAll = true
        root.__doExpandAll()
    }

    function collapseAll()
    {
        root.resetVerticalScrollPosition()

        // collapse all, except for the root path - from the last item (leaves) up to the root
        for (let nRow = root.rows - 1; nRow >= 0; --nRow) {
            let index = root.__modelIndex(nRow)
            // we don't want to collapse the root path, because doing so will hide the contents
            // of the tree.
            if (assetsModel.filePath(index) === assetsModel.rootPath())
                break

            root.collapse(nRow)
        }
    }

    // workaround for a bug -- might be fixed by https://codereview.qt-project.org/c/qt/qtdeclarative/+/442721
    onContentHeightChanged: {
        if (root.contentHeight <= root.height) {
            let first = root.itemAtCell(0, root.firstRow)
            if (!first)
                root.contentY = 0
        }
    }

    function computeAllExpandedState()
    {
        var dirsWithChildren = [...Array(root.rows).keys()].filter(row => {
            let index = root.__modelIndex(row)
            return assetsModel.isDirectory(index) && assetsModel.hasChildren(index)
        })

        var countExpanded = dirsWithChildren.filter(row => root.isExpanded(row)).length

        if (countExpanded === dirsWithChildren.length)
            return "all_expanded"

        if (countExpanded === 0)
            return "all_collapsed"
        return ""
    }

    function startDropHoverOver(row)
    {
        let index = root.__modelIndex(row)
        if (assetsModel.isDirectory(index)) {
            let item = root.__getDelegateItemForIndex(index)
            if (item)
                item.isHighlighted = true
            return
        }

        let parentItem = root.__getDelegateParentForIndex(index)
        if (parentItem)
            parentItem.hasChildWithDropHover = true
    }

    function endDropHover(row)
    {
        let index = root.__modelIndex(row)
        if (assetsModel.isDirectory(index)) {
            let item = root.__getDelegateItemForIndex(index)
            if (item)
                item.isHighlighted = false
            return
        }

        let parentItem = root.__getDelegateParentForIndex(index)
        if (parentItem)
            parentItem.hasChildWithDropHover = false
    }

    function isAssetSelected(itemPath)
    {
        return root.selectedAssets[itemPath] ? true : false
    }

    function clearSelectedAssets()
    {
        root.selectedAssets = {}
    }

    function setAssetSelected(itemPath, selected)
    {
        root.selectedAssets[itemPath] = selected
        root.selectedAssetsChanged()
    }

    function __getDelegateParentForIndex(index)
    {
        let parentIndex = assetsModel.parentDirIndex(index)
        let parentCell = root.cellAtIndex(parentIndex)
        return root.itemAtCell(parentCell)
    }

    function __getDelegateItemForIndex(index)
    {
        let cell = root.cellAtIndex(index)
        return root.itemAtCell(cell)
    }

    function __modelIndex(row)
    {
        // The modelIndex() function exists since 6.3. In Qt 6.3, this modelIndex() function was a
        // member of the TreeView, while in Qt6.4 it was moved to TableView. In Qt 6.4, the order of
        // the arguments was changed, and in Qt 6.5 the order was changed again. Due to this mess,
        // the whole function was deprecated in Qt 6.4.3 and replaced with index() function.
        if (assetsRoot.qtVersion >= 0x060403)
            return root.index(row, 0)
        else if (assetsRoot.qtVersion >= 0x060400)
            return root.modelIndex(0, row)
        else
            return root.modelIndex(row, 0)
    }

    function __selectRow(row: int)
    {
        let index = root.__modelIndex(row)
        if (assetsModel.isDirectory(index))
            return

        let filePath = assetsModel.filePath(index)

        root.clearSelectedAssets()
        root.setAssetSelected(filePath, true)
        root.currentFilePath = filePath
    }

    function moveSelection(amount)
    {
        if (!assetsModel.haveFiles || !amount)
            return

        let index = root.currentFilePath ? assetsModel.indexForPath(root.currentFilePath)
                                         : root.__modelIndex(root.firstRow)
        let row = root.rowAtIndex(index)
        let nextRow = row
        let nextIndex = index

        do {
            nextRow = nextRow + amount
            if ((amount < 0 && nextRow < root.firstRow) || (amount > 0 && nextRow > root.lastRow))
                return
            nextIndex = root.__modelIndex(nextRow)
        } while (assetsModel.isDirectory(nextIndex))

        root.__selectRow(nextRow)
        root.positionViewAtRow(nextRow, TableView.Contain)
    }

    Keys.enabled: true

    Keys.onUpPressed: {
        moveSelection(-1)
    }

    Keys.onDownPressed: {
        moveSelection(1)
    }

    ConfirmDeleteFilesDialog {
        id: confirmDeleteFiles
        parent: root
        files: []

        onAccepted: root.clearSelectedAssets()
        onClosed: confirmDeleteFiles.files = []
    }

    DropArea {
        id: dropArea
        enabled: true
        anchors.fill: parent

        property bool __isHoveringDrop: false
        property int __rowHoveringOver: -1

        function __rowAndItem(drag)
        {
            let pos = dropArea.mapToItem(root, drag.x, drag.y)
            let cell = root.cellAtPos(pos.x, pos.y, true)
            let item = root.itemAtCell(cell)

            return [cell.y, item]
        }

        onEntered: (drag) => {
            root.assetsRoot.updateDropExtFiles(drag)

            let [row, item] = dropArea.__rowAndItem(drag)
            dropArea.__isHoveringDrop = drag.accepted && root.assetsRoot.dropSimpleExtFiles.length > 0

            if (item && dropArea.__isHoveringDrop)
                root.startDropHoverOver(row)

            dropArea.__rowHoveringOver = row
        }

        onDropped: (drag) => {
            let [row, item] = dropArea.__rowAndItem(drag)

            if (item) {
                drag.accept()
                root.endDropHover(row)

                let dirPath = item.getDirPath()

                rootView.emitExtFilesDrop(root.assetsRoot.dropSimpleExtFiles,
                                          root.assetsRoot.dropComplexExtFiles,
                                          dirPath)
            }

            dropArea.__isHoveringDrop = false
            dropArea.__rowHoveringOver = -1
        }

        onPositionChanged: (drag) => {
            let [row, item] = dropArea.__rowAndItem(drag)

            if (dropArea.__rowHoveringOver !== row && dropArea.__rowHoveringOver > -1) {
                root.endDropHover(dropArea.__rowHoveringOver)

                if (item)
                    root.startDropHoverOver(row)
            }

            dropArea.__rowHoveringOver = row
        }

        onExited: {
            if (!dropArea.__isHoveringDrop || dropArea.__rowHoveringOver === -1)
                return

            root.endDropHover(dropArea.__rowHoveringOver)

            dropArea.__isHoveringDrop = false
            dropArea.__rowHoveringOver = -1
        }
    }

    delegate: AssetDelegate {
        assetsView: root
        assetsRoot: root.assetsRoot
        indentation: 5
    }
} // TreeView
