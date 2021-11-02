/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

/* The view displaying the item grid.

The following Qml context properties have to be set:
- ItemLibraryModel  itemLibraryModel
- int               itemLibraryIconWidth
- int               itemLibraryIconHeight
- ItemLibraryWidget rootView
- QColor            highlightColor

itemLibraryModel structure:

itemLibraryModel [
    ItemLibraryImport {
        string importName
        string importUrl
        bool importVisible
        bool importUsed
        bool importExpanded

        list categoryModel [
            ItemLibraryCategory {
                string categoryName
                bool categoryVisible
                bool categoryExpanded

                list itemModel [
                    ItemLibraryItem {
                        string itemName
                        string itemLibraryIconPath
                        bool itemVisible
                        string componentPath
                        var itemLibraryEntry
                    },
                    ... more items
                ]
            },
            ... more categories
        ]
    },
    ... more imports
]
*/

Item {
    id: itemsView

    property string importToRemove: ""
    property string importToAdd: ""
    property var currentCategory: null
    property var currentImport: null
    property bool isHorizontalView: false

    // Called also from C++ to close context menu on focus out
    function closeContextMenu()
    {
        moduleContextMenu.close()
        itemContextMenu.close()
    }

    onWidthChanged: {
        itemsView.isHorizontalView = itemsView.width > widthLimit
    }

    onIsHorizontalViewChanged: closeContextMenu()

    Item {
        id: styleConstants
        property int textWidth: 58
        property int textHeight: Theme.smallFontPixelSize() * 2

        property int cellHorizontalMargin: 1
        property int cellVerticalSpacing: 2
        property int cellVerticalMargin: 4

        // the following depend on the actual shape of the item delegate
        property int cellWidth: styleConstants.textWidth + 2 * styleConstants.cellHorizontalMargin
        property int cellHeight: itemLibraryIconHeight + styleConstants.textHeight +
                                 2 * styleConstants.cellVerticalMargin + styleConstants.cellVerticalSpacing

        StudioControls.Menu {
            id: moduleContextMenu

            StudioControls.MenuItem {
                text: qsTr("Remove Module")
                visible: itemsView.currentCategory === null
                height: visible ? implicitHeight : 0
                enabled: itemsView.importToRemove !== "" && !rootView.subCompEditMode
                onTriggered: rootView.removeImport(itemsView.importToRemove)
            }

            StudioControls.MenuSeparator {
                visible: itemsView.currentCategory === null
                height: StudioTheme.Values.border
            }

            StudioControls.MenuItem {
                text: qsTr("Expand All")
                visible: itemsView.currentCategory === null
                height: visible ? implicitHeight : 0
                onTriggered: itemLibraryModel.expandAll()
            }

            StudioControls.MenuItem {
                text: qsTr("Collapse All")
                visible: itemsView.currentCategory === null
                height: visible ? implicitHeight : 0
                onTriggered: itemLibraryModel.collapseAll()
            }

            StudioControls.MenuSeparator {
                visible: itemsView.currentCategory === null
                height: StudioTheme.Values.border
            }

            StudioControls.MenuItem {
                text: qsTr("Hide Category")
                visible: itemsView.currentCategory
                height: visible ? implicitHeight : 0
                onTriggered: itemLibraryModel.hideCategory(itemsView.currentImport.importUrl,
                                                           itemsView.currentCategory.categoryName)
            }

            StudioControls.MenuSeparator {
                visible: itemsView.currentCategory
                height: StudioTheme.Values.border
            }

            StudioControls.MenuItem {
                text: qsTr("Show Module Hidden Categories")
                enabled: itemsView.currentImport && !itemsView.currentImport.allCategoriesVisible
                onTriggered: itemLibraryModel.showImportHiddenCategories(itemsView.currentImport.importUrl)
            }

            StudioControls.MenuItem {
                text: qsTr("Show All Hidden Categories")
                enabled: itemLibraryModel.isAnyCategoryHidden
                onTriggered: itemLibraryModel.showAllHiddenCategories()
            }
        }

        StudioControls.Menu {
            id: itemContextMenu
            // Workaround for menu item implicit width not properly propagating to menu
            width: importMenuItem.implicitWidth

            StudioControls.MenuItem {
                id: importMenuItem
                text: qsTr("Add Module: ") + itemsView.importToAdd
                enabled: itemsView.importToAdd !== ""
                onTriggered: rootView.addImportForItem(itemsView.importToAdd)
            }
        }
    }

    Loader {
        anchors.fill: parent
        sourceComponent: itemsView.isHorizontalView ? horizontalView : verticalView
    }

    Component {
        id: verticalView

        ScrollView {
            id: verticalScrollView
            width: itemsView.width
            height: itemsView.height
            onContentHeightChanged: {
                var maxPosition = Math.max(contentHeight - verticalScrollView.height, 0)
                if (contentY > maxPosition)
                    contentY = maxPosition
            }

            Column {
                spacing: 2
                Repeater {
                    model: itemLibraryModel  // to be set in Qml context
                    delegate: Section {
                        width: itemsView.width -
                               (verticalScrollView.verticalScrollBarVisible
                                ? verticalScrollView.verticalThickness : 0)
                        caption: importName
                        visible: importVisible
                        sectionHeight: 30
                        sectionFontSize: 15
                        showArrow: categoryModel.rowCount() > 0
                        labelColor: importUnimported ? StudioTheme.Values.themeUnimportedModuleColor
                                                     : StudioTheme.Values.themeTextColor
                        leftPadding: 0
                        rightPadding: 0
                        expanded: importExpanded
                        expandOnClick: false
                        useDefaulContextMenu: false
                        onToggleExpand: {
                            if (categoryModel.rowCount() > 0)
                                importExpanded = !importExpanded
                        }
                        onShowContextMenu: {
                            itemsView.importToRemove = importRemovable ? importUrl : ""
                            itemsView.currentImport = model
                            itemsView.currentCategory = null
                            if (!rootView.isSearchActive())
                                moduleContextMenu.popup()
                        }

                        Column {
                            spacing: 2
                            property var currentImportModel: model // allows accessing the import model from inside the category section
                            Repeater {
                                model: categoryModel
                                delegate: Section {
                                    width: itemsView.width -
                                           (verticalScrollView.verticalScrollBarVisible
                                           ? verticalScrollView.verticalThickness : 0)
                                    sectionBackgroundColor: "transparent"
                                    showTopSeparator: index > 0
                                    hideHeader: categoryModel.rowCount() <= 1
                                    leftPadding: 0
                                    rightPadding: 0
                                    addTopPadding: categoryModel.rowCount() > 1
                                    addBottomPadding: index !== categoryModel.rowCount() - 1
                                    caption: categoryName + " (" + itemModel.rowCount() + ")"
                                    visible: categoryVisible
                                    expanded: categoryExpanded
                                    expandOnClick: false
                                    onToggleExpand: categoryExpanded = !categoryExpanded
                                    useDefaulContextMenu: false
                                    onShowContextMenu: {
                                        itemsView.currentCategory = model
                                        itemsView.currentImport = parent.currentImportModel
                                        if (!rootView.isSearchActive())
                                            moduleContextMenu.popup()
                                    }

                                    Grid {
                                        id: itemGrid

                                        property real actualWidth: parent.width - itemGrid.leftPadding -itemGrid.rightPadding
                                        property int flexibleWidth: (itemGrid.actualWidth / columns) - styleConstants.cellWidth

                                        leftPadding: 6
                                        rightPadding: 6
                                        columns: itemGrid.actualWidth / styleConstants.cellWidth
                                        rowSpacing: 7

                                        Repeater {
                                            model: itemModel
                                            delegate: ItemDelegate {
                                                visible: itemVisible
                                                textColor: importUnimported ? StudioTheme.Values.themeUnimportedModuleColor
                                                                            : StudioTheme.Values.themeTextColor
                                                width: styleConstants.cellWidth + itemGrid.flexibleWidth
                                                height: styleConstants.cellHeight
                                                onShowContextMenu: {
                                                    if (!itemUsable) {
                                                        itemsView.importToAdd = itemRequiredImport
                                                        itemContextMenu.popup()
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: horizontalView

        Row {
            padding: 5

            ScrollView {
                id: horizontalScrollView
                width: 270
                height: itemsView.height
                onContentHeightChanged: {
                    var maxPosition = Math.max(contentHeight - horizontalScrollView.height, 0)
                    if (contentY > maxPosition)
                        contentY = maxPosition
                }

                Column {
                    width: parent.width
                    spacing: 2
                    Repeater {
                        model: itemLibraryModel  // to be set in Qml context
                        delegate: Section {
                            width: 265 -
                                   (horizontalScrollView.verticalScrollBarVisible
                                   ? horizontalScrollView.verticalThickness : 0)
                            caption: importName
                            visible: importVisible
                            sectionHeight: 30
                            sectionFontSize: 15
                            showArrow: categoryModel.rowCount() > 0
                            labelColor: importUnimported ? StudioTheme.Values.themeUnimportedModuleColor
                                                         : StudioTheme.Values.themeTextColor
                            leftPadding: 0
                            rightPadding: 0
                            expanded: importExpanded
                            expandOnClick: false
                            useDefaulContextMenu: false
                            onToggleExpand: {
                                if (categoryModel.rowCount() > 0)
                                    importExpanded = !importExpanded
                            }
                            onShowContextMenu: {
                                itemsView.importToRemove = importRemovable ? importUrl : ""
                                itemsView.currentImport = model
                                itemsView.currentCategory = null
                                if (!rootView.isSearchActive())
                                    moduleContextMenu.popup()
                            }

                            Column {
                                spacing: 2
                                property var currentImportModel: model // allows accessing the import model from inside the category section
                                Repeater {
                                    model: categoryModel
                                    delegate: Rectangle {
                                        width: 265 -
                                               (horizontalScrollView.verticalScrollBarVisible
                                               ? horizontalScrollView.verticalThickness : 0)
                                        height: 25
                                        visible: categoryVisible
                                        border.width: StudioTheme.Values.border
                                        border.color: StudioTheme.Values.themeControlOutline
                                        color: categorySelected
                                               ? StudioTheme.Values.themeControlBackgroundHover
                                               : categoryMouseArea.containsMouse ? Qt.darker(StudioTheme.Values.themeControlBackgroundHover, 1.5)
                                                                                 : StudioTheme.Values.themeControlBackground

                                        Text {
                                            anchors.fill: parent
                                            text: categoryName
                                            color: StudioTheme.Values.themeTextColor
                                            font.pixelSize: 13
                                            font.capitalization: Font.AllUppercase
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }

                                        MouseArea {
                                            id: categoryMouseArea
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            acceptedButtons: Qt.LeftButton | Qt.RightButton

                                            onClicked: (mouse) => {
                                                itemLibraryModel.selectImportCategory(parent.parent.currentImportModel.importUrl, model.index)

                                                if (mouse.button === Qt.RightButton && !rootView.isSearchActive() && categoryModel.rowCount() !== 1) {
                                                    itemsView.currentCategory = model
                                                    itemsView.currentImport = parent.parent.currentImportModel
                                                    moduleContextMenu.popup()
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle { // separator between import/category column and item grid
                id: separatingLine
                height: itemsView.height - 10
                width: 1
                color: StudioTheme.Values.themeControlOutline
            }

            ScrollView {
                id: itemScrollView
                width: itemsView.width - 275
                height: itemsView.height
                onContentHeightChanged: {
                    var maxPosition = Math.max(contentHeight - itemScrollView.height, 0)
                    if (contentY > maxPosition)
                        contentY = maxPosition
                }

                Grid {
                    id: hItemGrid
                    property real actualWidth: itemsView.width - 294
                    property int flexibleWidth: (hItemGrid.actualWidth / hItemGrid.columns) - styleConstants.cellWidth

                    leftPadding: 9
                    rightPadding: 9
                    bottomPadding: 15
                    columns: hItemGrid.actualWidth / styleConstants.cellWidth
                    rowSpacing: 7

                    Repeater {
                        model: itemLibraryModel.itemsModel
                        delegate: ItemDelegate {
                            visible: itemVisible
                            textColor: itemLibraryModel.importUnimportedSelected
                                       ? StudioTheme.Values.themeUnimportedModuleColor : StudioTheme.Values.themeTextColor
                            width: styleConstants.cellWidth + hItemGrid.flexibleWidth
                            height: styleConstants.cellHeight
                            onShowContextMenu: {
                                if (!itemUsable) {
                                    itemsView.importToAdd = itemRequiredImport
                                    itemContextMenu.popup()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
