// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import AssetsLibraryBackend

Item {
    id: root

    property var assetsModel: AssetsLibraryBackend.assetsModel
    property var rootView: AssetsLibraryBackend.rootView

    // Array of supported externally dropped files that are imported as-is
    property var dropSimpleExtFiles: []

    // Array of supported externally dropped files that trigger custom import process
    property var dropComplexExtFiles: []

    readonly property int qtVersion: rootView.qtVersion()
    property bool __searchBoxEmpty: true

    AssetsContextMenu {
        id: contextMenu
        assetsView: assetsView
    }

    function clearSearchFilter() {
        searchBox.clear()
    }

    function updateDropExtFiles(drag) {
        root.dropSimpleExtFiles = []
        root.dropComplexExtFiles = []
        var simpleSuffixes = rootView.supportedAssetSuffixes(false)
        var complexSuffixes = rootView.supportedAssetSuffixes(true)
        for (const u of drag.urls) {
            var url = u.toString()
            if (assetsModel.urlPathExistsInModel(url))
                continue;

            var ext = '*.' + url.slice(url.lastIndexOf('.') + 1).toLowerCase()
            if (simpleSuffixes.includes(ext))
                root.dropSimpleExtFiles.push(url)
            else if (complexSuffixes.includes(ext))
                root.dropComplexExtFiles.push(url)
        }

        drag.accepted = root.dropSimpleExtFiles.length > 0 || root.dropComplexExtFiles.length > 0
    }

    DropArea { // handles external drop on empty area of the view (goes to root folder)
        id: dropArea
        y: assetsView.y + assetsView.contentHeight - assetsView.rowSpacing
        width: parent.width
        height: parent.height - y

        onEntered: (drag) => {
            root.updateDropExtFiles(drag)
        }

        onDropped: (drag) => {
            drag.accept()
            rootView.handleExtFilesDrop(root.dropSimpleExtFiles,
                                        root.dropComplexExtFiles,
                                        assetsModel.rootPath())
        }

        Canvas { // marker for the drop area
            id: dropCanvas
            y: 5
            width: parent.width
            height: parent.height - y
            visible: dropArea.containsDrag && root.dropSimpleExtFiles.length > 0

            onWidthChanged: dropCanvas.requestPaint()
            onHeightChanged: dropCanvas.requestPaint()

            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                ctx.strokeStyle = StudioTheme.Values.themeInteraction
                ctx.lineWidth = 2
                ctx.setLineDash([4, 4])
                ctx.rect(5, 5, dropCanvas.width - 10, dropCanvas.height - 10)
                ctx.stroke()
            }
        }
    }

    MouseArea { // right clicking the empty area of the view
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: {
            if (assetsModel.haveFiles) {
                function onFolderCreated(path) {
                    assetsView.addCreatedFolder(path)
                }

                var rootIndex = assetsModel.rootIndex()
                var dirPath = assetsModel.filePath(rootIndex)
                var dirName = assetsModel.fileName(rootIndex)
                contextMenu.openContextMenuForRoot(rootIndex, dirPath, dirName, onFolderCreated)
            }
        }
    }

    // called from C++ to close context menu on focus out
    function handleViewFocusOut() {
        contextMenu.close()
        assetsView.selectedAssets = {}
        assetsView.selectedAssetsChanged()
    }

    Timer {
        id: updateSearchFilterTimer
        interval: 200
        repeat: false

        onTriggered: {
            assetsView.resetVerticalScrollPosition()
            rootView.handleSearchFilterChanged(searchBox.text)
            assetsView.expandAll()

            if (root.__searchBoxEmpty && searchBox.text)
                root.__searchBoxEmpty = false
            else if (!root.__searchBoxEmpty && !searchBox.text)
                root.__searchBoxEmpty = true
        }
    }

    Column {
        id: column
        anchors.fill: parent
        spacing: 5

        Rectangle {
            id: toolbar
            width: parent.width
            height: StudioTheme.Values.doubleToolbarHeight
            color: StudioTheme.Values.themeToolbarBackground

            Column {
                id: toolbarColumn
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
                    onSearchChanged: (searchText) => {
                        updateSearchFilterTimer.restart()
                    }
                }

                Row {
                    id: buttonRow
                    width: parent.width
                    height: StudioTheme.Values.toolbarHeight
                    spacing: 6

                    HelperWidgets.AbstractButton {
                        id: addModuleButton
                        style: StudioTheme.Values.viewBarButtonStyle
                        buttonIcon: StudioTheme.Constants.add_medium
                        tooltip: qsTr("Add a new asset to the project.")
                        onClicked: rootView.handleAddAsset()
                    }
                }
            }
        }

        Text {
            text: qsTr("No match found.")
            leftPadding: 10
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.baseFont
            visible: !assetsModel.haveFiles && !root.__searchBoxEmpty
        }

        Item { // placeholder when the assets library is empty
            width: parent.width
            height: parent.height - toolbar.height - column.spacing
            visible: !assetsModel.haveFiles && root.__searchBoxEmpty
            clip: true

            MouseArea { // right clicking the empty area of the view
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                onClicked: {
                    contextMenu.openContextMenuForEmpty(assetsModel.rootPath())
                }
            }

            DropArea { // handles external drop (goes into default folder based on suffix)
                anchors.fill: parent

                onEntered: (drag) => {
                    root.updateDropExtFiles(drag)
                }

                onDropped: (drag) => {
                    drag.accept()
                    rootView.emitExtFilesDrop(root.dropSimpleExtFiles, root.dropComplexExtFiles)
                }

                Column {
                    id: noAssetsColumn
                    spacing: 20
                    width: root.width - 40
                    anchors.centerIn: parent

                    Text {
                        text: qsTr("Looks like you don't have any assets yet.")
                        color: StudioTheme.Values.themeTextColor
                        font.pixelSize: StudioTheme.Values.bigFont
                        width: noAssetsColumn.width
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }

                    Image {
                        source: "image://qmldesigner_assets/browse"
                        anchors.horizontalCenter: parent.horizontalCenter
                        scale: maBrowse.containsMouse ? 1.2 : 1

                        Behavior on scale {
                            NumberAnimation {
                                duration: 300
                                easing.type: Easing.OutQuad
                            }
                        }

                        MouseArea {
                            id: maBrowse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: rootView.handleAddAsset();
                        }
                    }

                    Text {
                        text: qsTr("Drag-and-drop your assets here or click the '+' button to browse assets from the file system.")
                        color: StudioTheme.Values.themeTextColor
                        font.pixelSize: StudioTheme.Values.bigFont
                        width: noAssetsColumn.width
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }

        AssetsView {
            id: assetsView

            width: parent.width
            height: parent.height - assetsView.y

            assetsRoot: root
            contextMenu: contextMenu
            focus: true
        }
    }
}
