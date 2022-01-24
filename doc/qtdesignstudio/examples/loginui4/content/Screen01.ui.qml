import QtQuick
import QtQuick.Controls
import Loginui1
import QtQuick.Timeline 1.0

Rectangle {
    id: rectangle
    width: Constants.width
    height: Constants.height

    color: Constants.backgroundColor
    state: "login"

    Image {
        id: adventurePage
        anchors.fill: parent
        source: "images/adventurePage.jpg"
        fillMode: Image.PreserveAspectFit
    }

    Image {
        id: qt_logo_green_128x128px
        anchors.top: parent.top
        source: "images/qt_logo_green_128x128px.png"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 40
        fillMode: Image.PreserveAspectFit
    }
    Text {
        id: tagLine
        color: "#ffffff"
        text: qsTr("Are you ready to explore?")
        anchors.top: qt_logo_green_128x128px.bottom
        font.pixelSize: 50
        anchors.topMargin: 40
        anchors.horizontalCenter: parent.horizontalCenter
        font.family: "Titillium Web ExtraLight"
        anchors.horizontalCenterOffset: 0
    }

    EntryField {
        id: username
        text: qsTr("Username or Email")
        anchors.top: tagLine.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 170
    }

    EntryField {
        id: password
        text: qsTr("Password")
        anchors.top: username.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 21
    }

    EntryField {
        id: repeatPassword
        opacity: 0
        text: qsTr("Repeat Password")
        anchors.top: password.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 20
    }

    Column {
        id: buttons
        y: 944
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 50
        spacing: 20

        PushButton {
            id: login
            text: qsTr("Continue")
        }

        PushButton {
            id: createAccount
            text: qsTr("Create Account")

            Connections {
                target: createAccount
                onClicked: rectangle.state = "createAccount"
            }
        }
    }

    Timeline {
        id: timeline
        animations: [
            TimelineAnimation {
                id: toCreateAccountState
                duration: 1000
                running: false
                loops: 1
                to: 1000
                from: 0
            }
        ]
        endFrame: 1000
        enabled: true
        startFrame: 0

        KeyframeGroup {
            target: repeatPassword
            property: "opacity"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 1
                frame: 1000
            }
        }

        KeyframeGroup {
            target: createAccount
            property: "opacity"
            Keyframe {
                value: 1
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1000
            }
        }

        KeyframeGroup {
            target: repeatPassword
            property: "anchors.topMargin"
            Keyframe {
                value: -100
                frame: 0
            }

            Keyframe {
                value: -100
                frame: 4
            }

            Keyframe {
                easing.bezierCurve: [0.39, 0.575, 0.565, 1, 1, 1]
                value: 20
                frame: 999
            }
        }
    }
    states: [
        State {
            name: "login"
        },
        State {
            name: "createAccount"

            PropertyChanges {
                target: timeline
                enabled: true
            }

            PropertyChanges {
                target: toCreateAccountState
                running: true
            }
        }
    ]
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.5}D{i:1}D{i:2}D{i:3}D{i:4}D{i:5}D{i:6}D{i:8}D{i:10}D{i:9}D{i:7}
D{i:11}
}
##^##*/

