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

    property bool headerDocked: false
    readonly property Item headerItem: headerDocked ? dockedHeaderLoader.item : undockedHeaderLoader.item

    property Component headerComponent: null

    // Called from C++ to close context menu on focus out
    function closeContextMenu() {
        Controller.closeContextMenu()
    }

    Loader {
        id: dockedHeaderLoader

        anchors.top: itemPane.top
        z: parent.z + 1
        height: item ? item.implicitHeight : 0
        width: parent.width

        active: itemPane.headerDocked
        sourceComponent: itemPane.headerComponent

        HeaderBackground{}
    }

    MouseArea {
        anchors.fill: parent
        onClicked: itemPane.forceActiveFocus()
    }

    Rectangle {
        id: stateSection
        width: itemPane.width
        height: StudioTheme.Values.height + StudioTheme.Values.controlGap * 2
        color:  StudioTheme.Values.themePanelBackground
        z: isBaseState ? -1: 10
        SectionLayout {
            y: StudioTheme.Values.controlGap
            x: StudioTheme.Values.controlGap
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
                        anchors.leftMargin: StudioTheme.Values.inputHorizontalPadding
                        anchors.topMargin: StudioTheme.Values.typeLabelVerticalShift
                        text: stateName
                        color: StudioTheme.Values.themeInteraction
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

    HelperWidgets.ScrollView {
        id: mainScrollView

        clip: true
        anchors {
            top: dockedHeaderLoader.bottom
            bottom: itemPane.bottom
            left: itemPane.left
            right: itemPane.right
            topMargin: dockedHeaderLoader.active ? 2 : 0 + isBaseState ? 0 : stateSection.height
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
}
