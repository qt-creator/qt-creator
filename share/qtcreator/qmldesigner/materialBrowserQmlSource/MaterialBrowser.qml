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

    property var currMaterialItem: null

    // Called also from C++ to close context menu on focus out
    function closeContextMenu()
    {
        ctxMenu.close()
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
            if (materialBrowserModel.hasMaterialRoot || !materialBrowserModel.hasQuick3DImport)
                return;

            var matsSecBottom = mapFromItem(materialsSection, 0, materialsSection.y).y
                                + materialsSection.height;

            if (!materialBrowserModel.hasMaterialRoot && materialBrowserModel.hasQuick3DImport
                && mouse.y < matsSecBottom) {
                ctxMenu.popupMenu()
            }
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

    Column {
        id: col
        y: 5
        spacing: 5

        Row {
            width: root.width
            enabled: !materialBrowserModel.hasMaterialRoot && materialBrowserModel.hasQuick3DImport

            StudioControls.SearchBox {
                id: searchBox

                width: root.width - addMaterialButton.width

                onSearchChanged: (searchText) => {
                    rootView.handleSearchFilterChanged(searchText)
                }
            }

            IconButton {
                id: addMaterialButton

                tooltip: qsTr("Add a material.")

                icon: StudioTheme.Constants.plus
                anchors.verticalCenter: parent.verticalCenter
                buttonSize: searchBox.height
                onClicked: materialBrowserModel.addNewMaterial()
                enabled: materialBrowserModel.hasQuick3DImport
            }
        }

        Text {
            text: {
                if (materialBrowserModel.hasMaterialRoot)
                    qsTr("<b>Material Browser</b> is disabled inside a material component.")
                else if (!materialBrowserModel.hasQuick3DImport)
                    qsTr("To use <b>Material Browser</b>, first add the QtQuick3D module in the <b>Components</b> view.")
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
            visible: materialBrowserModel.hasQuick3DImport && !materialBrowserModel.hasMaterialRoot
            interactive: !ctxMenu.opened

            Column {
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
                        id: grid

                        width: scrollView.width
                        leftPadding: 5
                        rightPadding: 5
                        bottomPadding: 5
                        columns: root.width / root.cellWidth

                        Repeater {
                            id: materialRepeater

                            model: materialBrowserModel
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
                        visible: materialBrowserModel.isEmpty && !searchBox.isEmpty() && !materialBrowserModel.hasMaterialRoot
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

                Section {
                    id: texturesSection

                    width: root.width
                    caption: qsTr("Textures")

                    Grid {
                        width: scrollView.width
                        leftPadding: 5
                        rightPadding: 5
                        bottomPadding: 5
                        spacing: 5
                        columns: root.width / root.cellWidth

                        Repeater {
                            id: texturesRepeater

                            model: materialBrowserTexturesModel
                            delegate: TextureItem {
                                width: root.cellWidth
                                height: root.cellWidth

                                onShowContextMenu: {
//                                    ctxMenuTexture.popupMenu(this, model) // TODO: implement textures context menu
                                }
                            }
                        }
                    }

                    Text {
                        text: qsTr("No match found.");
                        color: StudioTheme.Values.themeTextColor
                        font.pixelSize: StudioTheme.Values.baseFontSize
                        leftPadding: 10
                        visible: materialBrowserModel.isEmpty && !searchBox.isEmpty() && !materialBrowserModel.hasMaterialRoot
                    }

                    Text {
                        text:qsTr("There are no texture in this project.")
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
        }
    }
}
