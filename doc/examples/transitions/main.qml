//! [2]

//! [1]

//! [0]

import QtQuick 2.0

Rectangle {
    id: page
    width: 360
    height: 360
    color: "#343434"

    Image {
        id: icon
        x: 10
        y: 20
        source: "states.png"
    }

    Rectangle {
        id: topLeftRect
        y: 20
        width: 64
        height: 64
        color: "#00000000"
        radius: 6
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.top: parent.top
        anchors.topMargin: 20
        border.color: "#808080"

        MouseArea {
            id: mousearea1
            anchors.fill: parent
            onClicked: page.state = ' '
            }
        }
//! [0]

        Rectangle {
            id: middleRightRect
            x: 13
            y: 16
            width: 64
            height: 64
            color: "#00000000"
            radius: 6
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            border.color: "#808080"
            MouseArea {
                id: mousearea2
                anchors.fill: parent
                onClicked: page.state = 'State1'
            }
        }

        Rectangle {
            id: bottomLeftRect
            y: 11
            width: 64
            height: 64
            color: "#00000000"
            radius: 6
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20
            border.color: "#808080"
            MouseArea {
                id: mousearea3
                anchors.fill: parent
                onClicked: page.state = 'State2'
            }
        }

//! [1]

        states: [
             State {
                 name: "State1"

                 PropertyChanges {
                     target: icon
                     x: middleRightRect.x
                     y: middleRightRect.y
                 }
             },
             State {
                 name: "State2"

                 PropertyChanges {
                     target: icon
                     x: bottomLeftRect.x
                     y: bottomLeftRect.y
                 }
             }
         ]

        transitions: [
        Transition {
                from: "*"; to: "State1"
                NumberAnimation {
                    easing.type: Easing.OutBounce
                    properties: "x,y";
                    duration: 1000
                }
            },
        Transition {
                from: "*"; to: "State2"
                NumberAnimation {
                    properties: "x,y";
                    easing.type: Easing.InOutQuad;
                    duration: 2000
                }
            },
        Transition {
                 NumberAnimation {
                     properties: "x,y";
                     duration: 200
                 }
             }
        ]
}

//! [2]
