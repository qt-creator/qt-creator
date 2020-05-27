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
            color: "#d4d4d4"
            x: 14
            y: 5
            width: 100 //Bit of black magic to define the default size
            height: 40

            border.color: "gray"
            border.width: 1
            radius: 2
            anchors.fill: parent //binding has to be preserved
            source: "assets/buttonNormal.png"

            Text {
                id: normalText
                x: 58
                y: 50 //id only required to preserve binding
                text: control.text //binding has to be preserved
                //anchors.fill: parent
                color: "#BBBBBB"
                font.letterSpacing: 0.594
                font.pixelSize: 24
            }
        }

        Image {
            id: buttonPressed
            x: 290
            y: 5
            width: 100 //Bit of black magic to define the default size
            height: 40
            source: "assets/buttonPressed.png"

            border.color: "gray"
            border.width: 1
            radius: 2
            anchors.fill: parent //binding has to be preserved

            Text {
                id: pressedText //id only required to preserve binding
                x: 58
                y: 50
                text: control.text //binding has to be preserved
                //anchors.fill: parent
                color: "#E1E1E1"

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

