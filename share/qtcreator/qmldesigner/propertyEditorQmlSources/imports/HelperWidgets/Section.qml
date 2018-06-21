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

import QtQuick 2.1
import QtQuick.Controls 1.1 as Controls
import QtQuick.Layouts 1.0
import QtQuickDesignerTheme 1.0

Item {
    id: section
    property alias caption: label.text
    property int leftPadding: 8
    property int topPadding: 4
    property int rightPadding: 0

    property int animationDuration: 0

    property bool expanded: true

    clip: true

    Rectangle {
        id: header
        height: 20

        anchors.left: parent.left
        anchors.right: parent.right

        Controls.Label {
            id: label
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.color(Theme.PanelTextColorLight)
            x: 22
            font.bold: true
        }

        Image {
            id: arrow
            width: 8
            height: 4
            source: "image://icons/down-arrow"
            anchors.left: parent.left
            anchors.leftMargin: 4
            anchors.verticalCenter: parent.verticalCenter
            Behavior on rotation {
                NumberAnimation {
                    easing.type: Easing.OutCubic
                    duration: animationDuration
                }
            }

        }

        color: Theme.color(Theme.BackgroundColorDark)

        Rectangle {
            visible: false
            color:"#333"
            width: parent.width
            height: 1
        }

        Rectangle {
            visible: false
            color: "#333"
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                section.animationDuration = 120
                section.expanded = !section.expanded
            }
        }
    }

    default property alias __content: row.children

    readonly property alias contentItem: row

    implicitHeight: Math.round(row.height + header.height + 8)

    Row {
        anchors.left: parent.left
        anchors.leftMargin: leftPadding
        anchors.right: parent.right
        anchors.rightMargin: rightPadding
        anchors.top: header.bottom
        anchors.topMargin: topPadding
        id: row
    }

    Behavior on implicitHeight {
        NumberAnimation {
            easing.type: Easing.OutCubic
            duration: animationDuration
        }
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
}
