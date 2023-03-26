// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.4

ListView {
    currentIndex: -1
    id: list_view1
    anchors.fill: parent
    focus: true
    section.criteria: ViewSection.FirstCharacter
    section.property: "fullName"
    section.delegate : Rectangle {
        height: childrenRect.height
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#5ca4fb"
            }

            GradientStop {
                position: 1
                color: "#2d548b"
            }
        }
        width: parent.width
        Text {
            x: 12
            color: "#ffffff"
            text: section
            style: Text.Raised
            font.pixelSize: 18
            font.bold: true
        }
    }

    delegate: Item {
        clip: true
        state:  ListView.isCurrentItem ? "expanded" : "collapsed"
        id: delegate
        x: 5
        height: 50
        width: parent.width
        Behavior on height {
            PropertyAnimation {

            }
        }

        Column {
            id: column1
            height: 400
            width: parent.width - 20
            spacing: 4


            Row {
                id: row1
                spacing: 10
                Image {
                    source: "dummy.png"

                }

                Text {
                    text: fullName
                    font.pointSize: 16
                    anchors.verticalCenter: parent.verticalCenter
                    font.bold: true
                }
            }
            Grid {
                opacity: delegate.state === "expanded" ? 1 : 0
                Behavior on opacity {
                    PropertyAnimation {

                    }
                }
                x: 60
                spacing: 10
                columns: 2
                Text {
                    text: qsTr("Address:")
                    font.bold: true
                    font.pixelSize: 16
                }

                Text {
                    text: address
                    font.pixelSize: 16
                    font.bold: true
                }

                Text {
                    text: qsTr("City:")
                    font.pixelSize: 16
                    font.bold: true
                }

                Text {
                    text: city
                    font.pixelSize: 16
                    font.bold: true
                }

                Text {
                    text: qsTr("Number:")
                    font.pixelSize: 16
                    font.bold: true
                }

                Text {
                    text: number
                    font.pixelSize: 16
                    font.bold: true
                }
            }
            Rectangle {
                opacity: delegate.state === "expanded" ? 1 : 0
                Behavior on opacity {
                    PropertyAnimation {

                    }
                }
                height: 1
                color: "#828181"
                anchors.right: parent.right
                anchors.rightMargin: 4
                anchors.left: parent.left
                anchors.leftMargin: 4
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (delegate.ListView.view.currentIndex == index)
                    delegate.ListView.view.currentIndex = -1;
                else
                    delegate.ListView.view.currentIndex = index;
            }
        }

        states: [
            State {
                name: "collapsed"
            },
            State {
                name: "expanded"

                PropertyChanges {
                    target: delegate
                    height: 160
                }

            }
        ]
    }
}
