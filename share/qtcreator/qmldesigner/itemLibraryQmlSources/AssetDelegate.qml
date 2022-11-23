// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import StudioTheme as StudioTheme

TreeViewDelegate {
    id: root

    required property Item assetsView
    required property Item assetsRoot

    property bool hasChildWithDropHover: false
    property bool isHoveringDrop: false
    readonly property string suffix: model.fileName.substr(-4)
    readonly property bool isFont: root.suffix === ".ttf" || root.suffix === ".otf"
    readonly property bool isEffect: root.suffix === ".qep"
    property bool currFileSelected: false
    property int initialDepth: -1
    property bool _isDirectory: assetsModel.isDirectory(model.filePath)
    property int _currentRow: model.index
    property string _itemPath: model.filePath

    readonly property int _fileItemHeight: thumbnailImage.height
    readonly property int _dirItemHeight: 21

    implicitHeight: root._isDirectory ? root._dirItemHeight : root._fileItemHeight
    implicitWidth: root.assetsView.width > 0 ? root.assetsView.width : 10

    leftMargin: root._isDirectory ? 0 : thumbnailImage.width

    Component.onCompleted: {
        // the depth of the root path will become available before we get to the actual
        // items we display, so it's safe to set assetsView.rootPathDepth here. All other
        // tree items (below the root) will have the indentation (basically, depth) adjusted.
        if (model.filePath === assetsModel.rootPath()) {
            root.assetsView.rootPathDepth = root.depth
            root.assetsView.rootPathRow = root._currentRow
        } else if (model.filePath.includes(assetsModel.rootPath())) {
            root.depth -= root.assetsView.rootPathDepth
            root.initialDepth = root.depth
        }
    }

    // workaround for a bug -- might be fixed by https://codereview.qt-project.org/c/qt/qtdeclarative/+/442721
    onYChanged: {
        if (root._currentRow === root.assetsView.firstRow) {
            if (root.y > root.assetsView.contentY) {
                let item = root.assetsView.itemAtCell(0, root.assetsView.rootPathRow)
                if (!item)
                    root.assetsView.contentY = root.y
            }
        }
    }

    onImplicitWidthChanged: {
        // a small hack, to fix a glitch: when resizing the width of the tree view,
        // the widths of the delegate items remain the same as before, unless we re-set
        // that width explicitly.
        var newWidth = root.implicitWidth - (root.assetsView.verticalScrollBar.scrollBarVisible
                                             ? root.assetsView.verticalScrollBar.width
                                             : 0)
        bg.width = newWidth
        bg.implicitWidth = newWidth
    }

    onDepthChanged: {
        if (root.depth > root.initialDepth && root.initialDepth >= 0)
            root.depth = root.initialDepth
    }

    background: Rectangle {
        id: bg

        color: {
            if (root._isDirectory && (root.isHoveringDrop || root.hasChildWithDropHover))
                return StudioTheme.Values.themeInteraction

            if (!root._isDirectory && root.assetsView.selectedAssets[root._itemPath])
                return StudioTheme.Values.themeInteraction

            if (mouseArea.containsMouse)
                return StudioTheme.Values.themeSectionHeadBackground

            return root._isDirectory
                    ? StudioTheme.Values.themeSectionHeadBackground
                    : "transparent"
        }

        // this rectangle exists so as to have some visual indentation for the directories
        // We prepend a default pane-colored rectangle so that the nested directory will
        // look moved a bit to the right
        Rectangle {
            anchors.top: bg.top
            anchors.bottom: bg.bottom
            anchors.left: bg.left

            width: root.indentation * root.depth
            implicitWidth: root.indentation * root.depth
            color: StudioTheme.Values.themePanelBackground
        }
    }

    contentItem: Text {
        id: assetLabel
        text: assetLabel._computeText()
        color: StudioTheme.Values.themeTextColor
        font.pixelSize: 14
        anchors.verticalCenter: parent.verticalCenter
        verticalAlignment: Qt.AlignVCenter

        function _computeText()
        {
            return root._isDirectory
                    ? (root.hasChildren
                       ? model.display.toUpperCase()
                       : model.display.toUpperCase() + qsTr(" (empty)"))
                    : model.display
        }
    }

    DropArea {
        id: treeDropArea

        enabled: true
        anchors.fill: parent

        onEntered: (drag) => {
            root.assetsRoot.updateDropExtFiles(drag)
            root.isHoveringDrop = drag.accepted && root.assetsRoot.dropSimpleExtFiles.length > 0
            if (root.isHoveringDrop)
                root.assetsView.startDropHoverOver(root._currentRow)
        }

        onDropped: (drag) => {
            root.isHoveringDrop = false
            root.assetsView.endDropHover(root._currentRow)

            let dirPath = root._isDirectory
                       ? model.filePath
                       : assetsModel.parentDirPath(model.filePath);

            rootView.emitExtFilesDrop(root.assetsRoot.dropSimpleExtFiles,
                                      root.assetsRoot.dropComplexExtFiles,
                                      dirPath)
        }

        onExited: {
            if (root.isHoveringDrop) {
                root.isHoveringDrop = false
                root.assetsView.endDropHover(root._currentRow)
            }
        }
    }

    MouseArea {
        id: mouseArea

        property bool allowTooltip: true

        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onExited: tooltipBackend.hideTooltip()
        onEntered: mouseArea.allowTooltip = true

        onCanceled: {
            tooltipBackend.hideTooltip()
            mouseArea.allowTooltip = true
        }

        onPositionChanged: tooltipBackend.reposition()

        onPressed: (mouse) => {
            forceActiveFocus()
            mouseArea.allowTooltip = false
            tooltipBackend.hideTooltip()

            if (root._isDirectory)
                return

            var ctrlDown = mouse.modifiers & Qt.ControlModifier
            if (mouse.button === Qt.LeftButton) {
               if (!root.assetsView.isAssetSelected(root._itemPath) && !ctrlDown)
                   root.assetsView.clearSelectedAssets()
               root.currFileSelected = ctrlDown ? !root.assetsView.isAssetSelected(root._itemPath) : true
               root.assetsView.setAssetSelected(root._itemPath, root.currFileSelected)

               if (root.currFileSelected) {
                   let selectedPaths = root.assetsView.selectedPathsAsList()
                   rootView.startDragAsset(selectedPaths, mapToGlobal(mouse.x, mouse.y))
               }
            } else {
               if (!root.assetsView.isAssetSelected(root._itemPath) && !ctrlDown)
                   root.assetsView.clearSelectedAssets()
               root.currFileSelected = root.assetsView.isAssetSelected(root._itemPath) || !ctrlDown
               root.assetsView.setAssetSelected(root._itemPath, root.currFileSelected)
            }
        }

        onReleased: (mouse) => {
            mouseArea.allowTooltip = true

            if (mouse.button === Qt.LeftButton) {
                if (!(mouse.modifiers & Qt.ControlModifier))
                    root.assetsView.selectedAssets = {}
                root.assetsView.selectedAssets[root._itemPath] = root.currFileSelected
                root.assetsView.selectedAssetsChanged()
            }
        }

        onDoubleClicked: (mouse) => {
            forceActiveFocus()
            allowTooltip = false
            tooltipBackend.hideTooltip()
            if (mouse.button === Qt.LeftButton && isEffect)
                rootView.openEffectMaker(filePath)
        }

        ToolTip {
            visible: !root.isFont && mouseArea.containsMouse && !root.assetsView.contextMenu.visible
            text: model.filePath
            delay: 1000
        }

        Timer {
            interval: 1000
            running: mouseArea.containsMouse && mouseArea.allowTooltip
            onTriggered: {
                if (suffix === ".ttf" || suffix === ".otf") {
                    tooltipBackend.name = model.fileName
                    tooltipBackend.path = model.filePath
                    tooltipBackend.showTooltip()
                }
            }
        } // Timer

        onClicked: (mouse) => {
            if (mouse.button === Qt.LeftButton)
                root._toggleExpandCurrentRow()
            else
                root._openContextMenuForCurrentRow()


        }
    } // MouseArea

    function _openContextMenuForCurrentRow()
    {
        let modelIndex = assetsModel.indexForPath(model.filePath)

        if (root._isDirectory) {
            var row = root.assetsView.rowAtIndex(modelIndex)
            var expanded = root.assetsView.isExpanded(row)

            var allExpandedState = root.assetsView.computeAllExpandedState()

            function onFolderCreated(path) {
                root.assetsView.addCreatedFolder(path)
            }

            function onFolderRenamed() {
                if (expanded)
                    root.assetsView.rowToExpand = row
            }

            root.assetsView.contextMenu.openContextMenuForDir(modelIndex, model.filePath,
                model.fileName, allExpandedState, onFolderCreated, onFolderRenamed)
       } else {
            let parentDirIndex = assetsModel.parentDirIndex(model.filePath)
            let selectedPaths = root.assetsView.selectedPathsAsList()
            root.assetsView.contextMenu.openContextMenuForFile(modelIndex, parentDirIndex,
                                                               selectedPaths)
       }
    }

    function _toggleExpandCurrentRow()
    {
        if (!root._isDirectory)
           return

        let index = root.assetsView._modelIndex(root._currentRow, 0)
        // if the user manually clicked on a directory, then this is definitely not a
        // an automatic request to expand all.
        root.assetsView.requestedExpandAll = false

        if (root.assetsView.isExpanded(root._currentRow)) {
            root.assetsView.requestedExpandAll = false
            root.assetsView.collapse(root._currentRow)
        } else {
            root.assetsView.expand(root._currentRow)
        }
    }

    function reloadImage()
    {
        if (root._isDirectory)
            return

        thumbnailImage.source = ""
        thumbnailImage.source = thumbnailImage._computeSource()
    }

    Image {
        id: thumbnailImage
        visible: !root._isDirectory
        x: root.depth * root.indentation
        width: 48
        height: 48
        cache: false
        sourceSize.width: 48
        sourceSize.height: 48
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        source: thumbnailImage._computeSource()

        function _computeSource()
        {
            return root._isDirectory
                    ? ""
                    : "image://qmldesigner_assets/" + model.filePath
        }

    } // Image
} // TreeViewDelegate
