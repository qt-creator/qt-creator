/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.1
import QtQuick.Controls 1.1 as Controls
import QtQuick.Layouts 1.0

Item {

    id: section
    property alias caption: label.text

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

        gradient: Gradient {
            GradientStop {color: '#555' ; position: 0}
            GradientStop {color: '#444' ; position: 1}
        }

        Rectangle {
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

    default property alias __content: row.data

    readonly property alias contentItem: row

    implicitHeight: Math.round(row.height + header.height + 8)

    Row {
        width: parent.width
        x: 8
        y: header.height + 4
        id: row
        Behavior on opacity { NumberAnimation{easing.type: Easing.Linear ; duration: 80} }
    }

    Behavior on height { NumberAnimation{easing.type: Easing.OutCubic ; duration: 140} }

    states: [
        State {
            name: "Collapsed"
            PropertyChanges {
                target: section
                height: header.height
            }
            PropertyChanges {
                target: row
                opacity: 0

            }
        }
    ]

}
