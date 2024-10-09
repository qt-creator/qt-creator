// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import AssetsLibraryBackend

Item {
    id: root

    property var assetsModel: AssetsLibraryBackend.assetsModel
    property var rootView: AssetsLibraryBackend.rootView
    property var tooltipBackend: AssetsLibraryBackend.tooltipBackend

    required property Item assetsRoot
    required property StudioControls.Menu contextMenu
    // property alias verticalScrollBar: verticalScrollBar
    property var verticalScrollBar

    property var selectedAssets: ({})
    property var delegateItems: ({}) // all loaded asset delegates

    // the latest file that was clicked, or changed to via Up or Down keys
    property string currentFilePath: ""

    property var __createdDirectories: []

    property string currPath
    property bool reloadRootIndex: false

    property bool adsFocus: false
    // objectName is used by the dock widget to find this particular ScrollView
    // and set the ads focus on it.
    objectName: "__mainSrollView"

    x: 10 // padding

    function isFont(fileName)
    {
        let suffix = fileName.substr(-4)
        return suffix === ".ttf" || suffix === ".otf"
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
        root.updateSelectedAssets()
        rootView.assetSelected(itemPath)
    }

    function updateSelectedAssets()
    {
        for (var itemPath in root.delegateItems)
            root.delegateItems[itemPath].isSelected = root.selectedAssets[itemPath] || false
    }

    function selectedPathsAsList()
    {
        return Object.keys(root.selectedAssets).filter(itemPath => root.selectedAssets[itemPath])
    }

    function navToDir(path)
    {
        root.currPath = path
        root.reloadRootIndex = true
        gridView.model.rootIndex = root.assetsModel.indexForPath(root.currPath)
        root.updateDirEmptyState();
    }

    function updateDirEmptyState()
    {
        let dirEmpty = root.assetsModel.dirEmpty(root.currPath)
        emptyText.visible = dirEmpty
        emptyText.text = assetsRoot.searchBoxEmpty ? qsTr("Empty Folder.")
                                                   : qsTr("No match found.")
    }

    function resetVerticalScrollPosition()
    {
        // placeholder required by AssetsView.qml
    }

    function expandAll()
    {
        // placeholder required by AssetsView.qml
    }

    function addCreatedFolder(path)
    {
        // placeholder required by AssetsView.qml
    }

    Keys.onUpPressed: (e) => {
        if (e.modifiers & Qt.AltModifier) {
            e.accepted = true

            if (root.currPath.endsWith("/"))
                root.currPath = root.currPath.slice(0, -1);

            if (root.currPath === root.assetsModel.rootPath())
                return

            let lastSlashIndex = root.currPath.lastIndexOf("/");

            if (lastSlashIndex !== -1)
                navToDir(root.currPath.slice(0, lastSlashIndex + 1))
        }
    }

    ColumnLayout {
        anchors.fill: parent

        NavBar {
            id: navBar

            Layout.fillWidth: true

            currPath: root.currPath

            onPathClicked: (path) => root.navToDir(path)
        }

        GridView {
            id: gridView

            Layout.fillWidth: true
            Layout.fillHeight: true

            clip: true
            interactive: verticalScrollBar.visible && !root.contextMenu.opened && !rootView.isDragging
            reuseItems: false
            boundsBehavior: Flickable.StopAtBounds

            cellWidth: 80
            cellHeight: 98

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

            Timer {
                id: timer

                interval: 200
                repeat: false

                onTriggered: {
                    gridView.model.rootIndex = root.assetsModel.indexForPath(root.currPath)
                }
            }

            Connections {
                target: root.assetsModel

                function onDirectoryLoaded(path)
                {
                    root.updateDirEmptyState();

                    if (root.reloadRootIndex)
                        timer.start()
                }

                function onRootPathChanged()
                {
                    root.navToDir(root.assetsModel.rootPath())
                }

                function onFileChanged(filePath)
                {
                    rootView.invalidateThumbnail(filePath)
                    root.delegateItems[filePath].reloadImage()
                }

                function onModelResetFinished()
                {
                    root.navToDir(root.currPath)
                    root.updateDirEmptyState()
                }
            }

            model: DelegateModel {
                model: root.assetsModel

                delegate: Loader {
                    id: loader

                    width: gridView.cellWidth - 5
                    height: gridView.cellHeight - 5
                    visible: model.filePath.startsWith(root.assetsModel.rootPath()) // hide invalid items

                    sourceComponent: root.assetsModel.isDirectory(model.filePath) ? folderDelegate
                                                    : root.isFont(model.fileName) ? fontDelegate
                                                                                  : fileDelegate

                    onLoaded: {
                        loader.item.assetsView = root
                        loader.item.assetsRoot = root.assetsRoot
                        root.delegateItems[model.filePath] = loader.item;
                    }

                    Component {
                        id: folderDelegate

                        GridFolder {
                            width: parent.width
                            height: parent.height
                        }
                    }

                    Component {
                        id: fileDelegate

                        GridFile {
                            width: parent.width
                            height: parent.height
                        }
                    }

                    Component {
                        id: fontDelegate

                        GridFont {
                            width: parent.width
                            height: parent.height
                        }
                    }
                }
            } // DelegateModel
        } // GridView
    }

    Text {
        id: emptyText

        y: 40
        leftPadding: 10
        color: StudioTheme.Values.themeTextColor
        font.pixelSize: StudioTheme.Values.baseFont
        visible: false
    }

    MouseArea { // right clicking the empty area of the view
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: {
            if (root.assetsModel.hasFiles) {
                function onFolderCreated(path) {
                    assetsView.addCreatedFolder(path)
                }

                var currDirIndex = root.assetsModel.indexForPath(root.currPath)
                var dirName = assetsModel.fileName(currDirIndex)
                assetsRoot.contextMenu.openContextMenuForRoot(currDirIndex, root.currPath, dirName, onFolderCreated)
            }
        }
    }
}
