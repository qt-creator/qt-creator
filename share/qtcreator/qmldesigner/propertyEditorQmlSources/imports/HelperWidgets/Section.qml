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
import QtQuick.Controls 2.12 as Controls
import QtQuick.Layouts 1.15
import QtQuickDesignerTheme 1.0
import StudioTheme 1.0 as StudioTheme

Item {
    id: section
    property alias caption: label.text
    property alias sectionHeight: header.height
    property alias sectionBackgroundColor: header.color
    property alias sectionFontSize: label.font.pixelSize
    property alias showTopSeparator: topSeparator.visible
    property alias showArrow: arrow.visible

    property int leftPadding: 8
    property int topPadding: 4
    property int rightPadding: 0
    property int bottomPadding: 4

    property bool expanded: true
    property int level: 0
    property int levelShift: 10
    property bool hideHeader: false
    property bool expandOnClick: true // if false, toggleExpand signal will be emitted instead

    onHideHeaderChanged:
    {
        header.visible = !hideHeader
        header.height = hideHeader ? 0 : 20
    }

    clip: true

    signal showContextMenu()
    signal toggleExpand()

    Rectangle {
        id: header
        height: 20
        anchors.left: parent.left
        anchors.right: parent.right
        color: Qt.lighter(StudioTheme.Values.themeSectionHeadBackground, 1.0 + (0.2 * level))

        Image {
            id: arrow
            width: 8
            height: 4
            source: "image://icons/down-arrow"
            anchors.left: parent.left
            anchors.leftMargin: 4 + (level * levelShift)
            anchors.verticalCenter: parent.verticalCenter
        }

        Controls.Label {
            id: label
            anchors.verticalCenter: parent.verticalCenter
            color: StudioTheme.Values.themeTextColor
            x: 22 + (level * levelShift)
            font.bold: true
            font.pixelSize: StudioTheme.Values.myFontSize
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: {
                if (mouse.button === Qt.LeftButton) {
                    trans.enabled = true
                    if (expandOnClick)
                        expanded = !expanded
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
        anchors.rightMargin: 5 + leftPadding
        anchors.leftMargin: 5 - leftPadding
        visible: false
        color: "#666666"
    }

    default property alias __content: row.children

    readonly property alias contentItem: row

    implicitHeight: Math.round(row.height + header.height
                               + section.topPadding + section.bottomPadding)

    Row {
        id: row
        anchors.left: parent.left
        anchors.leftMargin: section.leftPadding
        anchors.right: parent.right
        anchors.rightMargin: section.rightPadding
        anchors.top: header.bottom
        anchors.topMargin: section.topPadding
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
            id: trans
            enabled: false
            NumberAnimation {
                properties: "implicitHeight,rotation";
                duration: 120;
                easing.type: Easing.OutCubic
            }
        }
}
