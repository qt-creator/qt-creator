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
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.15
import QtQuickDesignerTheme 1.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Item {
    id: section
    property alias caption: label.text
    property alias labelColor: label.color
    property alias sectionHeight: header.height
    property alias sectionBackgroundColor: header.color
    property alias sectionFontSize: label.font.pixelSize
    property alias showTopSeparator: topSeparator.visible
    property alias showArrow: arrow.visible
    property alias showLeftBorder: leftBorder.visible
    property alias spacing: column.spacing

    property int leftPadding: StudioTheme.Values.sectionLeftPadding
    property int rightPadding: 0
    property int topPadding: StudioTheme.Values.sectionHeadSpacerHeight
    property int bottomPadding: StudioTheme.Values.sectionHeadSpacerHeight

    property bool expanded: true
    property int level: 0
    property int levelShift: 10
    property bool hideHeader: false
    property bool expandOnClick: true // if false, toggleExpand signal will be emitted instead
    property bool addTopPadding: true
    property bool addBottomPadding: true

    property bool useDefaulContextMenu: true

    clip: true

    Connections {
        id: connection
        target: section
        enabled: section.useDefaulContextMenu
        function onShowContextMenu() { contextMenu.popup() }
    }

    Connections {
        target: Controller
        function onCollapseAll() { section.expanded = false }
        function onExpandAll() { section.expanded = true }
    }

    signal showContextMenu()
    signal toggleExpand()


    Rectangle {
        id: header
        height: section.hideHeader ? 0 : StudioTheme.Values.sectionHeadHeight
        visible: !section.hideHeader
        anchors.left: parent.left
        anchors.right: parent.right
        color: Qt.lighter(StudioTheme.Values.themeSectionHeadBackground, 1.0 + (0.2 * section.level))

        Item {
            StudioControls.Menu {
                id: contextMenu

                StudioControls.MenuItem {
                    text: qsTr("Expand All")
                    onTriggered: Controller.expandAll()
                }

                StudioControls.MenuItem {
                    text: qsTr("Collapse All")
                    onTriggered: Controller.collapseAll()
                }
            }
        }

        Image {
            id: arrow
            width: 8
            height: 4
            source: "image://icons/down-arrow"
            anchors.left: parent.left
            anchors.leftMargin: 4 + (section.level * section.levelShift)
            anchors.verticalCenter: parent.verticalCenter
        }

        Controls.Label {
            id: label
            anchors.verticalCenter: parent.verticalCenter
            color: StudioTheme.Values.themeTextColor
            x: 22 + (section.level * section.levelShift)
            font.pixelSize: StudioTheme.Values.myFontSize
            font.capitalization: Font.AllUppercase
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: function(mouse) {
                if (mouse.button === Qt.LeftButton) {
                    transition.enabled = true
                    if (section.expandOnClick)
                        section.expanded = !section.expanded
                    else
                        section.toggleExpand()
                } else {
                    section.showContextMenu()
                }
            }
        }
    }

    Rectangle {
        id: topSeparator
        height: 1
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.rightMargin: 5 + section.leftPadding
        anchors.leftMargin: 5 - section.leftPadding
        visible: false
        color: StudioTheme.Values.themeControlOutline
    }

    default property alias __content: column.children

    readonly property alias contentItem: column

    implicitHeight: Math.round(column.height + header.height + topSpacer.height + bottomSpacer.height)

    Item {
        id: topSpacer
        height: section.addTopPadding && column.height > 0 ? section.topPadding : 0
        anchors.top: header.bottom
    }

    Column {
        id: column
        anchors.left: parent.left
        anchors.leftMargin: section.leftPadding
        anchors.right: parent.right
        anchors.rightMargin: section.rightPadding
        anchors.top: topSpacer.bottom
    }

    Rectangle {
        id: leftBorder
        visible: false
        width: 1
        height: parent.height - 15
        color: header.color
    }

    Item {
        id: bottomSpacer
        height: section.addBottomPadding && column.height > 0 ? section.bottomPadding : 0
        anchors.top: column.bottom
    }

    states: [
        State {
            name: "Collapsed"
            when: !section.expanded
            PropertyChanges {
                target: section
                implicitHeight: header.height
            }
            PropertyChanges {
                target: arrow
                rotation: -90
            }
        }
    ]

    transitions: Transition {
        id: transition
        enabled: false
        NumberAnimation {
            properties: "implicitHeight,rotation"
            duration: 120
            easing.type: Easing.OutCubic
        }
    }
}
