/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

import QtQuick 2.1
import QtQuick.Controls 1.1 as Controls
import QtQuick.Layouts 1.0

Item {

    id: section
    property alias caption: label.text
    property int leftPadding: 8
    property int topPadding: 4
    property int rightPadding: 0

    clip: true

    Rectangle {
        id: header
        height: 18

        anchors.left: parent.left
        anchors.right: parent.right

        Controls.Label {
            id: label
            anchors.verticalCenter: parent.verticalCenter
            color: "white"
            x: 22
            style: Text.Sunken
            styleColor: "#292929"
            font.bold: true
        }

        Image {
            source: "images/down-arrow.png"
            rotation: section.state === "Collapsed" ? -90 : 0
            anchors.left: parent.left
            anchors.leftMargin: 4
            anchors.verticalCenter: parent.verticalCenter
            Behavior on rotation {NumberAnimation{duration: 80}}
        }

        color: "#444"

        Rectangle {
            visible: false
            color:"#333"
            width: parent.width
            height: 1
        }

        Rectangle {
            color: "#333"
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (section.state === "")
                    section.state = "Collapsed";
                else
                    section.state = "";
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
        Behavior on opacity { NumberAnimation{easing.type: Easing.Linear ; duration: 80} }
    }

    Behavior on height { NumberAnimation{easing.type: Easing.OutCubic ; duration: 140} }

    states: [
        State {
            name: "Collapsed"
            PropertyChanges {
                target: section
                implicitHeight: header.height
            }
            PropertyChanges {
                target: row
                opacity: 0

            }
        }
    ]

}
