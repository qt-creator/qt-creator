// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import ItemLibraryBackend
import QtQuickDesignerTheme
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

    property bool adsFocus: false
    // objectName is used by the dock widget to find this particular ScrollView
    // and set the ads focus on it.
    objectName: "__mainSrollView"

    property string importToRemove
    property string importToAdd
    property string componentSource
    property var currentCategory: null
    property var currentImport: null
    property bool isHorizontalView: false
    property bool isAddModuleView: false

    property var tooltipBackend: ItemLibraryBackend.tooltipBackend

    // Called also from C++ to close context menu on focus out
    function closeContextMenu()
    {
        moduleContextMenu.close()
        itemContextMenu.close()
    }
    // Called from C++
    function clearSearchFilter()
    {
        searchBox.clear();
    }
    // Called also from C++
    function switchToComponentsView()
    {
        ItemLibraryBackend.isAddModuleView = false
    }
    onWidthChanged: {
        itemsView.isHorizontalView = itemsView.width > ItemLibraryBackend.widthLimit
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
        property int cellHeight: ItemLibraryBackend.itemLibraryIconHeight + styleConstants.textHeight +
                                 2 * styleConstants.cellVerticalMargin + styleConstants.cellVerticalSpacing
        StudioControls.Menu {
            id: moduleContextMenu
            StudioControls.MenuItem {
                text: qsTr("Remove Module")
                visible: itemsView.currentCategory === null
                height: visible ? implicitHeight : 0
                enabled: itemsView.importToRemove && !ItemLibraryBackend.rootView.subCompEditMode
                onTriggered: ItemLibraryBackend.rootView.removeImport(itemsView.importToRemove)
            }
            StudioControls.MenuSeparator {
                visible: itemsView.currentCategory === null && !ItemLibraryBackend.rootView.searchActive
                height: visible ? StudioTheme.Values.border : 0
            }
            StudioControls.MenuItem {
                text: qsTr("Expand All")
                visible: itemsView.currentCategory === null && !ItemLibraryBackend.rootView.searchActive
                height: visible ? implicitHeight : 0
                onTriggered: ItemLibraryBackend.itemLibraryModel.expandAll()
            }
            StudioControls.MenuItem {
                text: qsTr("Collapse All")
                visible: itemsView.currentCategory === null && !ItemLibraryBackend.rootView.searchActive
                height: visible ? implicitHeight : 0
                onTriggered: ItemLibraryBackend.itemLibraryModel.collapseAll()
            }
            StudioControls.MenuSeparator {
                visible: itemsView.currentCategory === null && !ItemLibraryBackend.rootView.searchActive
                height: visible ? StudioTheme.Values.border : 0
            }
            StudioControls.MenuItem {
                text: qsTr("Hide Category")
                visible: itemsView.currentCategory
                height: visible ? implicitHeight : 0
                onTriggered: ItemLibraryBackend.itemLibraryModel.hideCategory(itemsView.currentImport.importUrl,
                                                           itemsView.currentCategory.categoryName)
            }
            StudioControls.MenuSeparator {
                visible: itemsView.currentCategory
                height: visible ? StudioTheme.Values.border : 0
            }
            StudioControls.MenuItem {
                text: qsTr("Show Module Hidden Categories")
                visible: !ItemLibraryBackend.rootView.searchActive
                enabled: itemsView.currentImport && !itemsView.currentImport.allCategoriesVisible
                height: visible ? implicitHeight : 0
                onTriggered: ItemLibraryBackend.itemLibraryModel.showImportHiddenCategories(itemsView.currentImport.importUrl)
            }
            StudioControls.MenuItem {
                text: qsTr("Show All Hidden Categories")
                visible: !ItemLibraryBackend.rootView.searchActive
                enabled: ItemLibraryBackend.itemLibraryModel.isAnyCategoryHidden
                height: visible ? implicitHeight : 0
                onTriggered: ItemLibraryBackend.itemLibraryModel.showAllHiddenCategories()
            }
        }
        StudioControls.Menu {
            id: itemContextMenu
            // Workaround for menu item implicit width not properly propagating to menu
            width: Math.max(importMenuItem.implicitWidth, openSourceItem.implicitWidth)
            StudioControls.MenuItem {
                id: importMenuItem
                text: qsTr("Add Module: ") + itemsView.importToAdd
                visible: itemsView.importToAdd
                height: visible ? implicitHeight : 0
                onTriggered: ItemLibraryBackend.rootView.addImportForItem(itemsView.importToAdd)
            }
            StudioControls.MenuItem {
                id: openSourceItem
                text: qsTr("Edit Component")
                visible: itemsView.componentSource
                height: visible ? implicitHeight : 0
                onTriggered: ItemLibraryBackend.rootView.goIntoComponent(itemsView.componentSource)
            }
        }
    }
    Column {
        id: col
        width: parent.width
        height: parent.height
        spacing: 5

        Rectangle {
            width: parent.width
            height: StudioTheme.Values.doubleToolbarHeight
            color: StudioTheme.Values.themeToolbarBackground

            Column {
                anchors.fill: parent
                anchors.topMargin: 6
                anchors.bottomMargin: 6
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 12

                StudioControls.SearchBox {
                    id: searchBox
                    width: parent.width
                    style: StudioTheme.Values.searchControlStyle

                    onSearchChanged: (searchText) => ItemLibraryBackend.rootView.handleSearchFilterChanged(searchText)
                }

                Row {
                    width: parent.width
                    height: StudioTheme.Values.toolbarHeight
                    spacing: 6

                    HelperWidgets.AbstractButton {
                        id: addModuleButton
                        style: StudioTheme.Values.viewBarButtonStyle
                        buttonIcon: StudioTheme.Constants.add_medium
                        tooltip: qsTr("Add a module.")
                        onClicked: isAddModuleView = true
                    }
                }
            }
        }

        Loader {
            id: loader
            width: col.width
            height: col.height - y - 5
            sourceComponent: isAddModuleView ? addModuleView
                                             : itemsView.isHorizontalView ? horizontalView : verticalView
        }
    }
    Component {
        id: verticalView
        HelperWidgets.ScrollView {
            id: verticalScrollView
            adsFocus: itemsView.adsFocus
            anchors.fill: parent
            clip: true
            interactive: !itemContextMenu.opened && !moduleContextMenu.opened && !ItemLibraryBackend.rootView.isDragging
            onContentHeightChanged: {
                var maxPosition = Math.max(contentHeight - verticalScrollView.height, 0)
                if (contentY > maxPosition)
                    contentY = maxPosition
            }
            Column {
                spacing: 2
                Repeater {
                    model: ItemLibraryBackend.itemLibraryModel  // to be set in Qml context
                    delegate: HelperWidgets.Section {
                        width: itemsView.width
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
                        category: "ItemsView"

                        onToggleExpand: {
                            if (categoryModel.rowCount() > 0)
                                importExpanded = !importExpanded
                        }
                        onShowContextMenu: {
                            itemsView.importToRemove = importRemovable ? importUrl : ""
                            itemsView.currentImport = model
                            itemsView.currentCategory = null
                            moduleContextMenu.popup()
                        }
                        Column {
                            spacing: 2
                            property var currentImportModel: model // allows accessing the import model from inside the category section
                            Repeater {
                                model: categoryModel
                                delegate: HelperWidgets.Section {
                                    width: itemsView.width
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
                                    category: "ItemsView"

                                    onShowContextMenu: {
                                        if (!ItemLibraryBackend.rootView.searchActive) {
                                            itemsView.currentCategory = model
                                            itemsView.currentImport = parent.currentImportModel
                                            moduleContextMenu.popup()
                                        }
                                    }
                                    Grid {
                                        id: itemGrid
                                        property real actualWidth: parent.width - itemGrid.leftPadding - itemGrid.rightPadding
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
                                                width: styleConstants.cellWidth
                                                height: styleConstants.cellHeight
                                                onShowContextMenu: {
                                                    if (!itemUsable || itemComponentSource) {
                                                        itemsView.importToAdd = !itemUsable ? itemRequiredImport : ""
                                                        itemsView.componentSource = itemComponentSource
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
            leftPadding: 5
            HelperWidgets.ScrollView {
                id: horizontalScrollView
                adsFocus: itemsView.adsFocus
                width: 270
                height: parent.height
                clip: true
                interactive: !itemContextMenu.opened && !moduleContextMenu.opened && !ItemLibraryBackend.rootView.isDragging

                onContentHeightChanged: {
                    var maxPosition = Math.max(contentHeight - horizontalScrollView.height, 0)
                    if (contentY > maxPosition)
                        contentY = maxPosition
                }
                Column {
                    width: parent.width
                    spacing: 2
                    Repeater {
                        model: ItemLibraryBackend.itemLibraryModel  // to be set in Qml context
                        delegate: HelperWidgets.Section {
                            width: 265
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
                            category: "ItemsView"

                            onToggleExpand: {
                                if (categoryModel.rowCount() > 0)
                                    importExpanded = !importExpanded
                            }
                            onShowContextMenu: {
                                itemsView.importToRemove = importRemovable ? importUrl : ""
                                itemsView.currentImport = model
                                itemsView.currentCategory = null
                                moduleContextMenu.popup()
                            }
                            Column {
                                spacing: 2
                                property var currentImportModel: model // allows accessing the import model from inside the category section
                                Repeater {
                                    model: categoryModel
                                    delegate: Rectangle {
                                        width: 265
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
                                                ItemLibraryBackend.itemLibraryModel.selectImportCategory(parent.parent.currentImportModel.importUrl, model.index)
                                                if (mouse.button === Qt.RightButton
                                                    && categoryModel.rowCount() !== 1
                                                    && !ItemLibraryBackend.rootView.searchActive) {
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
            HelperWidgets.ScrollView {
                id: itemScrollView
                adsFocus: itemsView.adsFocus
                width: itemsView.width - 275
                height: itemsView.height
                interactive: !itemContextMenu.opened && !moduleContextMenu.opened && !ItemLibraryBackend.rootView.isDragging

                onContentHeightChanged: {
                    var maxPosition = Math.max(contentHeight - itemScrollView.height, 0)
                    if (contentY > maxPosition)
                        contentY = maxPosition
                }
                Grid {
                    id: hItemGrid
                    property real actualWidth: itemsView.width - 294
                    leftPadding: 9
                    rightPadding: 9
                    bottomPadding: 15
                    columns: hItemGrid.actualWidth / styleConstants.cellWidth
                    rowSpacing: 7
                    Repeater {
                        model: ItemLibraryBackend.itemLibraryModel.itemsModel
                        delegate: ItemDelegate {
                            visible: itemVisible
                            textColor: ItemLibraryBackend.itemLibraryModel.importUnimportedSelected
                                       ? StudioTheme.Values.themeUnimportedModuleColor : StudioTheme.Values.themeTextColor
                            width: styleConstants.cellWidth
                            height: styleConstants.cellHeight
                            onShowContextMenu: {
                                if (!itemUsable || itemComponentSource) {
                                    itemsView.importToAdd = !itemUsable ? itemRequiredImport : ""
                                    itemsView.componentSource = itemComponentSource
                                    itemContextMenu.popup()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    Component {
        id: addModuleView
        AddModuleView {
            adsFocus: itemsView.adsFocus
            onBack: isAddModuleView = false
        }
    }
}
