// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Item {
    id: root

    // Array of supported externally dropped files that are imported as-is
    property var dropSimpleExtFiles: []

    // Array of supported externally dropped files that trigger custom import process
    property var dropComplexExtFiles: []

    readonly property int qtVersionAtLeast6_4: rootView.qtVersionIsAtLeast6_4()
    property bool __searchBoxEmpty: true

    AssetsContextMenu {
        id: contextMenu
        assetsView: assetsView
    }

    function clearSearchFilter()
    {
        searchBox.clear();
    }

    function updateDropExtFiles(drag)
    {
        root.dropSimpleExtFiles = []
        root.dropComplexExtFiles = []
        var simpleSuffixes = rootView.supportedAssetSuffixes(false);
        var complexSuffixes = rootView.supportedAssetSuffixes(true);
        for (const u of drag.urls) {
            var url = u.toString();
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
        y: assetsView.y + assetsView.contentHeight + 5
        width: parent.width
        height: parent.height - y

        onEntered: (drag)=> {
            root.updateDropExtFiles(drag)
        }

        onDropped: {
            rootView.handleExtFilesDrop(root.dropSimpleExtFiles, root.dropComplexExtFiles,
                                        assetsModel.rootPath())
        }

        Canvas { // marker for the drop area
            id: dropCanvas
            anchors.fill: parent
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
    function handleViewFocusOut()
    {
        contextMenu.close()
        assetsView.selectedAssets = {}
        assetsView.selectedAssetsChanged()
    }

    Column {
        anchors.fill: parent
        anchors.topMargin: 5
        spacing: 5

        Row {
            id: searchRow

            width: parent.width

            StudioControls.SearchBox {
                id: searchBox

                width: parent.width - addAssetButton.width - 5

                onSearchChanged: (searchText) => {
                    updateSearchFilterTimer.restart()
                }
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

            HelperWidgets.IconButton {
                id: addAssetButton
                anchors.verticalCenter: parent.verticalCenter
                tooltip: qsTr("Add a new asset to the project.")
                icon: StudioTheme.Constants.plus
                buttonSize: parent.height

                onClicked: rootView.handleAddAsset()
            }
        }

        Text {
            text: qsTr("No match found.")
            leftPadding: 10
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: 12
            visible: !assetsModel.haveFiles && !root.__searchBoxEmpty
        }

        Item { // placeholder when the assets library is empty
            width: parent.width
            height: parent.height - searchRow.height
            visible: !assetsModel.haveFiles && root.__searchBoxEmpty
            clip: true

            DropArea { // handles external drop (goes into default folder based on suffix)
                anchors.fill: parent

                onEntered: (drag)=> {
                    root.updateDropExtFiles(drag)
                }

                onDropped: {
                    rootView.emitExtFilesDrop(root.dropSimpleExtFiles, root.dropComplexExtFiles)
                }

                Column {
                    id: colNoAssets

                    spacing: 20
                    x: 20
                    width: root.width - 2 * x
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        text: qsTr("Looks like you don't have any assets yet.")
                        color: StudioTheme.Values.themeTextColor
                        font.pixelSize: 18
                        width: colNoAssets.width
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
                        font.pixelSize: 18
                        width: colNoAssets.width
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }

        AssetsView {
            id: assetsView
            assetsRoot: root
            contextMenu: contextMenu

            width: parent.width
            height: parent.height - y
        }
    } // Column
}
