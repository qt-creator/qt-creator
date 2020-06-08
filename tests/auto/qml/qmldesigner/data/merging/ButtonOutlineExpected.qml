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

        Rectangle {
            id: buttonNormal
            width: 100
            height: 60
            color: "#d4d4d4"
            radius: 2
            border.color: "#808080"
            border.width: 1
            anchors.fill: parent
            Text {
                id: normalText
                x: 33
                y: 24
                color: "#808080"
                text: control.text
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        Rectangle {
            id: buttonPressed
            width: 100
            height: 60
            color: "#69b5ec"
            radius: 2
            border.color: "#808080"
            border.width: 1
            anchors.fill: parent
            Text {
                id: pressedText
                x: 31
                y: 24
                color: "#000000"
                text: control.text
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
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
