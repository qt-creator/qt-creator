/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

import QtQuick 1.1

Item {
    id: root
    height:  48
    property alias text: text.text
    anchors.left: parent.left
    anchors.leftMargin: 20
    anchors.right: parent.right
    anchors.rightMargin: 20
    Rectangle {
        id: caption
        height: 32
        radius: 3
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#e6e6e6"
            }

            GradientStop {
                position: 1
                color: "#e4e4e4"
            }
        }
        smooth: true
        border.color: "#a4a4a4"
        anchors.left: parent.left
        anchors.right: parent.right
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 1
            anchors.leftMargin: 2
            anchors.rightMargin: 2
            height: 1
        }

        Text {
            id: text
            anchors.margins: 8
            color: "#19196f"
            text: "model.name"
            anchors.fill: parent
            wrapMode: Text.WordWrap
            styleColor: "#807b7b"
            style: Text.Normal
            font.pixelSize: 14
            font.bold: true
            textFormat: Text.PlainText
            maximumLineCount: 1
            elide: Text.ElideRight
        }
        states: [
            State {
                name: "hover"

                PropertyChanges {
                    target: caption
                    height: 48
                }
                PropertyChanges {
                    target: text
                    maximumLineCount: 2
                }
            }
        ]
        transitions: [
            Transition {
                SequentialAnimation {
                    PropertyAnimation {
                        target: caption
                        property: "height"
                        duration: 40
                    }
                    PropertyAction {
                        target: text
                        property: "maximumLineCount"
                    }
                }
            }
        ]
    }

    MouseArea {
        id: mouseArea
        x: 20
        y: 0
        anchors.fill: parent
        anchors.bottomMargin: 12
        hoverEnabled: true
        onEntered: {
            if (text.truncated) {
                caption.state = "hover"
            }
            root.parent.state = "hover"
        }
        onExited:{
            caption.state = ""
            root.parent.state = ""
        }
    }
}
