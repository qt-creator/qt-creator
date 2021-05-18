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

ScrollView {
    id: itemsView

    property string importToRemove: ""
    property string importToAdd: ""
    property var currentItem: null
    property var currentCategory: null
    property var currentImport: null

    // called from C++ to close context menu on focus out
    function closeContextMenu()
    {
        moduleContextMenu.close()
        itemContextMenu.close()
    }

    function showImportCategories()
    {
        currentImport.importCatVisibleState = true
        if (!itemLibraryModel.getIsAnyCategoryHidden())
            itemLibraryModel.isAnyCategoryHidden = false
    }

    onContentHeightChanged: {
        var maxPosition = Math.max(contentHeight - height, 0)
        if (contentY > maxPosition)
            contentY = maxPosition
    }

    Item {
        id: styleConstants
        property int textWidth: 58
        property int textHeight: Theme.smallFontPixelSize() * 2

        property int cellHorizontalMargin: 1
        property int cellVerticalSpacing: 2
        property int cellVerticalMargin: 4

        // the following depend on the actual shape of the item delegate
        property int cellWidth: textWidth + 2 * cellHorizontalMargin
        property int cellHeight: itemLibraryIconHeight + textHeight +
                                 2 * cellVerticalMargin + cellVerticalSpacing

        StudioControls.Menu {
            id: moduleContextMenu

            StudioControls.MenuItem {
                text: qsTr("Remove Module")
                visible: currentCategory === null
                height: visible ? implicitHeight : 0
                enabled: importToRemove !== ""
                onTriggered: {
                    showImportCategories()
                    rootView.removeImport(importToRemove)
                }
            }

            StudioControls.MenuSeparator {
                visible: currentCategory === null
                height: StudioTheme.Values.border
            }

            StudioControls.MenuItem {
                text: qsTr("Expand All")
                visible: currentCategory === null
                height: visible ? implicitHeight : 0
                onTriggered: itemLibraryModel.expandAll()
            }

            StudioControls.MenuItem {
                text: qsTr("Collapse All")
                visible: currentCategory === null
                height: visible ? implicitHeight : 0
                onTriggered: itemLibraryModel.collapseAll()
            }

            StudioControls.MenuSeparator {
                visible: currentCategory === null
                height: StudioTheme.Values.border
            }

            StudioControls.MenuItem {
                text: qsTr("Hide Category")
                visible: currentCategory
                height: visible ? implicitHeight : 0
                onTriggered: {
                    itemLibraryModel.isAnyCategoryHidden = true
                    currentCategory.categoryVisible = false
                }
            }

            StudioControls.MenuSeparator {
                visible: currentCategory
                height: StudioTheme.Values.border
            }

            StudioControls.MenuItem {
                text: qsTr("Show Module Hidden Categories")
                enabled: currentImport && !currentImport.importCatVisibleState
                onTriggered: showImportCategories()
            }

            StudioControls.MenuItem {
                text: qsTr("Show All Hidden Categories")
                enabled: itemLibraryModel.isAnyCategoryHidden
                onTriggered: {
                    itemLibraryModel.isAnyCategoryHidden = false
                    itemLibraryModel.showHiddenCategories()
                }
            }
        }

        StudioControls.Menu {
            id: itemContextMenu
            // Workaround for menu item implicit width not properly propagating to menu
            width: importMenuItem.implicitWidth

            StudioControls.MenuItem {
                id: importMenuItem
                text: qsTr("Add Module: ") + importToAdd
                enabled: importToAdd !== ""
                onTriggered: rootView.addImportForItem(importToAdd)
            }
        }
    }

    Column {
        spacing: 2
        Repeater {
            model: itemLibraryModel  // to be set in Qml context
            delegate: Section {
                width: itemsView.width -
                       (itemsView.verticalScrollBarVisible ? itemsView.verticalThickness : 0)
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
                onToggleExpand: {
                    if (categoryModel.rowCount() > 0)
                        importExpanded = !importExpanded
                }
                onShowContextMenu: {
                    importToRemove = importRemovable ? importUrl : ""
                    currentImport = model
                    currentCategory = null
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
                                   (itemsView.verticalScrollBarVisible ? itemsView.verticalThickness : 0)
                            sectionBackgroundColor: "transparent"
                            showTopSeparator: index > 0
                            hideHeader: categoryModel.rowCount() <= 1
                            leftPadding: 0
                            rightPadding: 0
                            addTopPadding: categoryModel.rowCount() > 1
                            addBottomPadding: index != categoryModel.rowCount() - 1
                            caption: categoryName + " (" + itemModel.rowCount() + ")"
                            visible: categoryVisible
                            expanded: categoryExpanded
                            expandOnClick: false
                            onToggleExpand: categoryExpanded = !categoryExpanded
                            onShowContextMenu: {
                                currentCategory = model
                                currentImport = parent.currentImportModel
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
                                                importToAdd = itemRequiredImport
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
