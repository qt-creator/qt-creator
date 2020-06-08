import QtQuick 2.10
import QtQuick.Templates 2.1 as T

T.Button {
    id: control

    implicitWidth: Math.max(
                       background ? background.implicitWidth : 0,
                       contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(
                        background ? background.implicitHeight : 0,
                        contentItem.implicitHeight + topPadding + bottomPadding)
    leftPadding: 4
    rightPadding: 4

    text: "My Button"

    background: Item {
        implicitWidth: buttonNormal.width
        implicitHeight: buttonNormal.height
        opacity: enabled ? 1 : 0.3

        Image {
            id: buttonNormal
            width: 100
            height: 40
            anchors.fill: parent
            source: "assets/buttonNormal.png"
            Text {
                id: normalText
                x: 58
                y: 50
                color: "#bbbbbb"
                text: control.text
                font.letterSpacing: 0.594
                font.pixelSize: 24
            }
        }

        Image {
            id: buttonPressed
            width: 100
            height: 40
            anchors.fill: parent
            source: "assets/buttonPressed.png"
            Text {
                id: pressedText
                x: 58
                y: 50
                color: "#e1e1e1"
                text: control.text
                font.letterSpacing: 0.594
                font.pixelSize: 24
            }
        }

    }

    contentItem: Item {}

    states: [
        State {
            name: "normal"
            when: !control.down
            PropertyChanges {
                target: buttonPressed
                visible: false
            }
            PropertyChanges {
                target: buttonNormal
                visible: true
            }
        },
        State {
            name: "down"
            when: control.down
            PropertyChanges {
                target: buttonPressed
                visible: true
            }
            PropertyChanges {
                target: buttonNormal
                visible: false
            }
        }
    ]
}

