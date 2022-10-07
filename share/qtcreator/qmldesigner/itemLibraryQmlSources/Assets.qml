/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Item {
    id: root

    property var selectedAssets: ({})
    property int allExpandedState: 0
    property string contextFilePath: ""
    property var contextDir: undefined
    property bool isDirContextMenu: false

    // Array of supported externally dropped files that are imported as-is
    property var dropSimpleExtFiles: []

    // Array of supported externally dropped files that trigger custom import process
    property var dropComplexExtFiles: []

    AssetsContextMenu {
        id: contextMenu
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
                                        assetsModel.rootDir().dirPath)
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
            if (!assetsModel.isEmpty) {
                root.contextFilePath = ""
                root.contextDir = assetsModel.rootDir()
                root.isDirContextMenu = false
                contextMenu.popup()
            }
        }
    }

    // called from C++ to close context menu on focus out
    function handleViewFocusOut()
    {
        contextMenu.close()
        root.selectedAssets = {}
        root.selectedAssetsChanged()
    }

    RegExpValidator {
        id: folderNameValidator
        regExp: /^(\w[^*/><?\\|:]*)$/
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

                onSearchChanged: (searchText) => rootView.handleSearchFilterChanged(searchText)
            }

            IconButton {
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
            visible: assetsModel.isEmpty && !searchBox.isEmpty()
        }


        Item { // placeholder when the assets library is empty
            width: parent.width
            height: parent.height - searchRow.height
            visible: assetsModel.isEmpty && searchBox.isEmpty()
            clip: true

            DropArea { // handles external drop (goes into default folder based on suffix)
                anchors.fill: parent

                onEntered: (drag)=> {
                    root.updateDropExtFiles(drag)
                }

                onDropped: {
                    rootView.handleExtFilesDrop(root.dropSimpleExtFiles, root.dropComplexExtFiles)
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
            width: parent.width
            height: parent.height - y
        }
    }
}
