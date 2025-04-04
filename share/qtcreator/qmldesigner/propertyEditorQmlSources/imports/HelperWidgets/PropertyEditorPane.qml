// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme

Rectangle {
    id: itemPane

    width: 320
    height: 400
    color: StudioTheme.Values.themePanelBackground

    Component.onCompleted: Controller.mainScrollView = mainScrollView

    default property alias content: mainColumn.children
    property alias scrollView: mainScrollView
    property alias searchBar: propertySearchBar

    property bool headerDocked: false
    readonly property Item headerItem: headerDocked ? dockedHeaderLoader.item : undockedHeaderLoader.item

    property Component headerComponent: null
    property alias toolbarComponent: toolbar.customToolbar

    // Called from C++ to close context menu on focus out
    function closeContextMenu() {
        Controller.closeContextMenu()
    }

    // Called from C++ to clear the search when the selected node changes
    function clearSearch() {
        propertySearchBar.clear();
    }

    PropertySearchBar {
        id: propertySearchBar

        contentItem: mainColumn
        anchors.top: itemPane.top
        width: parent.width
        z: parent.z + 1
    }

    PropertyEditorToolBar {
        id: toolbar

        anchors.top: propertySearchBar.bottom
        width: parent.width

        onToolBarAction: action => handleToolBarAction(action)
    }

    Loader {
        id: dockedHeaderLoader

        anchors.top: toolbar.bottom
        z: parent.z + 1
        height: item ? item.implicitHeight : 0
        width: parent.width

        active: itemPane.headerDocked
        sourceComponent: itemPane.headerComponent

        HeaderBackground{}
    }

    MouseArea {
        anchors.fill: mainScrollView
        onClicked: itemPane.forceActiveFocus()
    }

    HelperWidgets.ScrollView {
        id: mainScrollView

        clip: true
        anchors {
            top: dockedHeaderLoader.bottom
            bottom: itemPane.bottom
            left: itemPane.left
            right: itemPane.right
            topMargin: dockedHeaderLoader.active ? 2 : 0
        }

        interactive: !Controller.contextMenuOpened

        ColumnLayout {
            spacing: 2
            width: mainScrollView.width

            Loader {
                id: undockedHeaderLoader

                Layout.fillWidth: true
                Layout.preferredHeight: item ? item.implicitHeight : 0

                active: !itemPane.headerDocked
                sourceComponent: itemPane.headerComponent

                visible: active
                HeaderBackground{}
            }

            Loader {
                Layout.fillWidth: true
                Layout.leftMargin: StudioTheme.Values.sectionPadding
                Layout.preferredHeight: item ? item.implicitHeight : 0

                active: !isBaseState
                sourceComponent: StatesSection{}

                visible: active
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 10

                visible: propertySearchBar.hasDoneSearch && !propertySearchBar.hasMatchSearch
                text: qsTr("No match found.")
                color: StudioTheme.Values.themeTextColor
                font.pixelSize: StudioTheme.Values.baseFont
            }

            Column {
                id: mainColumn

                Layout.fillWidth: true

                onWidthChanged: StudioTheme.Values.responsiveResize(itemPane.width)
                Component.onCompleted: StudioTheme.Values.responsiveResize(itemPane.width)
            }
        }
    }

    component HeaderBackground: Rectangle {
        anchors.fill: parent
        anchors.leftMargin: -StudioTheme.Values.border
        anchors.rightMargin: -StudioTheme.Values.border
        z: parent.z - 1

        color: StudioTheme.Values.themeToolbarBackground
        border.color: StudioTheme.Values.themePanelBackground
        border.width: StudioTheme.Values.border
    }

    component StatesSection: SectionLayout {
        PropertyLabel {
            text: qsTr("Current State")
            tooltip: tooltipItem.tooltip
        }

        SecondColumnLayout {

            Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

            Item {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                height: StudioTheme.Values.height

                HelperWidgets.Label {
                    anchors.fill: parent
                    text: stateName
                    color: StudioTheme.Values.themeInteraction
                    verticalAlignment: Text.AlignVCenter
                }

                ToolTipArea {
                    id: tooltipItem

                    anchors.fill: parent
                    tooltip: qsTr("The current state of the States View.")
                }
            }

            ExpandingSpacer {}
        }
    }
}
