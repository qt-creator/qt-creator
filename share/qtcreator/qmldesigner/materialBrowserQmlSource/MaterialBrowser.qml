// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

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
    property var currentBundleMaterial: null
    property int copiedMaterialInternalId: -1

    property var matSectionsModel: []

    // Called also from C++ to close context menu on focus out
    function closeContextMenu()
    {
        cxtMenu.close()
        cxtMenuBundle.close()
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

        onClicked: (mouse) => {
            // root context-menu works only for user materials
            var userMatsSecBottom = mapFromItem(userMaterialsSection, 0, userMaterialsSection.y).y
                                    + userMaterialsSection.height;

            if (!materialBrowserModel.hasMaterialRoot && (!materialBrowserBundleModel.matBundleExists
                                                          || mouse.y < userMatsSecBottom)) {
                root.currentMaterial = null
                cxtMenu.popup()
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
        id: cxtMenu

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
                    onTriggered: {
                        root.copiedMaterialInternalId = root.currentMaterial.materialInternalId
                        materialBrowserModel.copyMaterialProperties(root.currentMaterialIdx, modelData)
                    }
                }
            }
        }

        StudioControls.MenuItem {
            text: qsTr("Paste properties")
            enabled: root.currentMaterial
                     && root.copiedMaterialInternalId !== root.currentMaterial.materialInternalId
                     && root.currentMaterial.materialType === materialBrowserModel.copiedMaterialType
                     && materialBrowserModel.isCopiedMaterialValid()
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

    StudioControls.Menu {
        id: cxtMenuBundle

        closePolicy: StudioControls.Menu.CloseOnEscape | StudioControls.Menu.CloseOnPressOutside

        StudioControls.MenuItem {
            text: qsTr("Apply to selected (replace)")
            enabled: root.currentBundleMaterial && materialBrowserModel.hasModelSelection
            onTriggered: materialBrowserBundleModel.applyToSelected(root.currentBundleMaterial, false)
        }

        StudioControls.MenuItem {
            text: qsTr("Apply to selected (add)")
            enabled: root.currentBundleMaterial && materialBrowserModel.hasModelSelection
            onTriggered: materialBrowserBundleModel.applyToSelected(root.currentBundleMaterial, true)
        }

        StudioControls.MenuSeparator {}

        StudioControls.MenuItem {
            text: qsTr("Add to project")

            onTriggered: materialBrowserBundleModel.addMaterial(root.currentBundleMaterial)
        }
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

                    // make sure searched categories that have matches are expanded
                    if (!materialBrowserModel.isEmpty && !userMaterialsSection.expanded)
                        userMaterialsSection.expanded = true

                    if (!materialBrowserBundleModel.isEmpty && !bundleMaterialsSection.expanded)
                        bundleMaterialsSection.expanded = true

                    for (let i = 0; i < bundleMaterialsSectionRepeater.count; ++i) {
                        let sec = bundleMaterialsSectionRepeater.itemAt(i)
                        if (sec.visible && !sec.expanded)
                            sec.expanded = true
                    }
                }
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

            Column {
                Section {
                    id: userMaterialsSection

                    width: root.width
                    caption: qsTr("User materials")
                    hideHeader: !materialBrowserBundleModel.matBundleExists

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
                                    root.currentMaterial = model
                                    cxtMenu.popup()
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
                    id: bundleMaterialsSection

                    width: root.width
                    caption: qsTr("Material Library")
                    addTopPadding: noMatchText.visible
                    visible: materialBrowserBundleModel.matBundleExists

                    Column {
                        Repeater {
                            id: bundleMaterialsSectionRepeater

                            model: materialBrowserBundleModel

                            delegate: Section {
                                width: root.width
                                caption: bundleCategory
                                addTopPadding: false
                                sectionBackgroundColor: "transparent"
                                visible: bundleCategoryVisible

                                Grid {
                                    width: scrollView.width
                                    leftPadding: 5
                                    rightPadding: 5
                                    bottomPadding: 5
                                    columns: root.width / root.cellWidth

                                    Repeater {
                                        model: bundleMaterialsModel

                                        delegate: BundleMaterialItem {
                                            width: root.cellWidth
                                            height: root.cellHeight

                                            onShowContextMenu: {
                                                root.currentBundleMaterial = modelData
                                                cxtMenuBundle.popup()
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Text {
                            id: noMatchText
                            text: qsTr("No match found.");
                            color: StudioTheme.Values.themeTextColor
                            font.pixelSize: StudioTheme.Values.baseFontSize
                            leftPadding: 10
                            visible: materialBrowserBundleModel.isEmpty && !searchBox.isEmpty() && !materialBrowserModel.hasMaterialRoot
                        }
                    }
                }
            }
        }
    }
}
