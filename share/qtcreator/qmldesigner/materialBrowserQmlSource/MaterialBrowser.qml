/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
import QtQuick.Layouts 1.15
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Item {
    id: root

    readonly property int cellWidth: 100
    readonly property int cellHeight: 120

    property var currentMaterial: null
    property int currentMaterialIdx: 0

    property var matSectionsModel: []

    // Called also from C++ to close context menu on focus out
    function closeContextMenu()
    {
        contextMenu.close()
    }

    // Called from C++ to refresh a preview material after it changes
    function refreshPreview(idx)
    {
        var item = gridRepeater.itemAt(idx);
        if (item)
            item.refreshPreview();
    }

    // Called from C++
    function clearSearchFilter()
    {
        searchBox.clear();
    }

    MouseArea {
        id: rootMouseArea

        anchors.fill: parent

        acceptedButtons: Qt.RightButton

        onClicked: {
            if (!materialBrowserModel.hasMaterialRoot) {
                root.currentMaterial = null
                contextMenu.popup()
            }
        }
    }

    Connections {
        target: materialBrowserModel

        function onSelectedIndexChanged() {
            // commit rename upon changing selection
            var item = gridRepeater.itemAt(currentMaterialIdx);
            if (item)
                item.commitRename();

            currentMaterialIdx = materialBrowserModel.selectedIndex;
        }
    }

    StudioControls.Menu {
        id: contextMenu

        closePolicy: StudioControls.Menu.CloseOnEscape | StudioControls.Menu.CloseOnPressOutside

        StudioControls.MenuItem {
            text: qsTr("Apply to selected (replace)")
            enabled: root.currentMaterial && materialBrowserModel.hasModelSelection
            onTriggered: materialBrowserModel.applyToSelected(root.currentMaterial.materialInternalId, false)
        }

        StudioControls.MenuItem {
            text: qsTr("Apply to selected (add)")
            enabled: root.currentMaterial && materialBrowserModel.hasModelSelection
            onTriggered: materialBrowserModel.applyToSelected(root.currentMaterial.materialInternalId, true)
        }

        StudioControls.MenuSeparator {
            height: StudioTheme.Values.border
        }

        StudioControls.Menu {
            title: qsTr("Copy properties")
            enabled: root.currentMaterial

            width: parent.width

            onAboutToShow: {
                root.matSectionsModel = ["All"];

                switch (root.currentMaterial.materialType) {
                case "DefaultMaterial":
                    root.matSectionsModel = root.matSectionsModel.concat(materialBrowserModel.defaultMaterialSections);
                    break;

                case "PrincipledMaterial":
                    root.matSectionsModel = root.matSectionsModel.concat(materialBrowserModel.principledMaterialSections);
                    break;

                case "CustomMaterial":
                    root.matSectionsModel = root.matSectionsModel.concat(materialBrowserModel.customMaterialSections);
                    break;
                }
            }

            Repeater {
                model: root.matSectionsModel

                StudioControls.MenuItem {
                    text: modelData
                    enabled: root.currentMaterial
                    onTriggered: materialBrowserModel.copyMaterialProperties(root.currentMaterialIdx, modelData)
                }
            }
        }

        StudioControls.MenuItem {
            text: qsTr("Paste properties")
            enabled: root.currentMaterial && root.currentMaterial.materialType
                                             === materialBrowserModel.copiedMaterialType
            onTriggered: materialBrowserModel.pasteMaterialProperties(root.currentMaterialIdx)
        }

        StudioControls.MenuSeparator {
            height: StudioTheme.Values.border
        }

        StudioControls.MenuItem {
            text: qsTr("Duplicate")
            enabled: root.currentMaterial
            onTriggered: materialBrowserModel.duplicateMaterial(root.currentMaterialIdx)
        }

        StudioControls.MenuItem {
            text: qsTr("Rename")
            enabled: root.currentMaterial
            onTriggered: {
                var item = gridRepeater.itemAt(root.currentMaterialIdx);
                if (item)
                    item.startRename();
            }
        }

        StudioControls.MenuItem {
            text: qsTr("Delete")
            enabled: root.currentMaterial

            onTriggered: materialBrowserModel.deleteMaterial(root.currentMaterialIdx)
        }

        StudioControls.MenuSeparator {}

        StudioControls.MenuItem {
            text: qsTr("Create New Material")

            onTriggered: materialBrowserModel.addNewMaterial()
        }
    }

    Column {
        id: col
        y: 5
        spacing: 5

        Row {
            width: root.width
            enabled: !materialBrowserModel.hasMaterialRoot && materialBrowserModel.hasQuick3DImport

            SearchBox {
                id: searchBox

                width: root.width - addMaterialButton.width
            }

            IconButton {
                id: addMaterialButton

                tooltip: qsTr("Add a material.")

                icon: StudioTheme.Constants.plus
                anchors.verticalCenter: parent.verticalCenter
                buttonSize: searchBox.height
                onClicked: materialBrowserModel.addNewMaterial()
            }
        }

        Text {
            text: qsTr("No match found.");
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.baseFontSize
            leftPadding: 10
            visible: materialBrowserModel.hasQuick3DImport && materialBrowserModel.isEmpty
                     && !searchBox.isEmpty() && !materialBrowserModel.hasMaterialRoot
        }

        Text {
            text: {
                if (materialBrowserModel.hasMaterialRoot)
                    qsTr("<b>Material Browser</b> is disabled inside a material component.")
                else if (!materialBrowserModel.hasQuick3DImport)
                    qsTr("To use <b>Material Browser</b>, first add the QtQuick3D module in the <b>Components</b> view.")
                else if (materialBrowserModel.isEmpty && searchBox.isEmpty())
                    qsTr("There are no materials in this project.<br>Select '<b>+</b>' to create one.")
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

            Grid {
                id: grid

                width: scrollView.width
                leftPadding: 5
                rightPadding: 5
                bottomPadding: 5
                columns: root.width / root.cellWidth

                Repeater {
                    id: gridRepeater

                    model: materialBrowserModel
                    delegate: MaterialItem {
                        width: root.cellWidth
                        height: root.cellHeight

                        onShowContextMenu: {
                            if (searchBox.isEmpty()) {
                                root.currentMaterial = model
                                contextMenu.popup()
                            }
                        }
                    }
                }
            }
        }
    }
}
