import QtQuick 2.14

Rectangle {
    id: page
    width: 640
    height: 480
    visible: true
    border.color: "#808080"
    state: "State1"

    Image {
        id: icon
        x: 10
        y: 20
        source: "qt-logo.png"
        fillMode: Image.PreserveAspectFit
    }

    Rectangle {
        id: topLeftRect
        width: 55
        height: 41
        color: "#00000000"
        border.color: "#808080"
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 10
        anchors.topMargin: 20

        MouseArea {
            id: mouseArea
            anchors.fill: parent

            Connections {
                target: mouseArea
                onClicked: page.state = "State1"
            }
        }
    }

    Rectangle {
        id: middleRightRect
        width: 55
        height: 41
        color: "#00000000"
        border.color: "#808080"
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 10
        MouseArea {
            id: mouseArea1
            anchors.fill: parent

            Connections {
                target: mouseArea1
                onClicked: page.state = "State2"
            }
        }
    }

    Rectangle {
        id: bottomLeftRect
        width: 55
        height: 41
        color: "#00000000"
        border.color: "#808080"
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        MouseArea {
            id: mouseArea2
            anchors.fill: parent

            Connections {
                target: mouseArea2
                onClicked: page.state = "State3"
            }
        }
        anchors.leftMargin: 10
    }
    states: [
        State {
            name: "State1"

            PropertyChanges {
                target: icon
                x: 10
                y: 20
            }
        },
        State {
            name: "State2"

            PropertyChanges {
                target: icon
                x: 575
                y: 219
            }
        },
        State {
            name: "State3"
            PropertyChanges {
                target: icon
                x: 10
                y: 419
            }
        }
    ]
    transitions: [
        Transition {
            id: toState1
            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 50
                    }

                    PropertyAnimation {
                        target: icon
                        property: "x"
                        easing.bezierCurve: [0.2,0.2,0.8,0.8,1,1]
                        duration: 152
                    }
                }

                SequentialAnimation {
                    PauseAnimation {
                        duration: 52
                    }

                    PropertyAnimation {
                        target: icon
                        property: "y"
                        easing.bezierCurve: [0.2,0.2,0.8,0.8,1,1]
                        duration: 152
                    }
                }
            }
            to: "State1"
            from: "State2,State3"
        },
        Transition {
            id: toState2
            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 50
                    }

                    PropertyAnimation {
                        target: icon
                        property: "x"
                        easing.bezierCurve: [0.233,0.161,0.264,0.997,0.393,0.997,0.522,0.997,0.555,0.752,0.61,0.75,0.664,0.748,0.736,1,0.775,1,0.814,0.999,0.861,0.901,0.888,0.901,0.916,0.901,0.923,0.995,1,1]
                        duration: 951
                    }
                }

                SequentialAnimation {
                    PauseAnimation {
                        duration: 50
                    }

                    PropertyAnimation {
                        target: icon
                        property: "y"
                        easing.bezierCurve: [0.233,0.161,0.264,0.997,0.393,0.997,0.522,0.997,0.555,0.752,0.61,0.75,0.664,0.748,0.736,1,0.775,1,0.814,0.999,0.861,0.901,0.888,0.901,0.916,0.901,0.923,0.995,1,1]
                        duration: 951
                    }
                }
            }
            to: "State2"
            from: "State1,State3"
        },
        Transition {
            id: toState3
            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: icon
                        property: "x"
                        easing.bezierCurve: [0.25,0.46,0.45,0.94,1,1]
                        duration: 2000
                    }
                }

                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: icon
                        property: "y"
                        easing.bezierCurve: [0.25,0.46,0.45,0.94,1,1]
                        duration: 2000
                    }
                }
            }
            to: "State3"
            from: "State1,State2"
        }
    ]
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.6600000262260437}D{i:17;transitionDuration:2000}D{i:25;transitionDuration:2000}
D{i:33;transitionDuration:2000}
}
##^##*/
