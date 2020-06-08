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
            color: "#d4d4d4"
            width: 100 //Bit of black magic to define the default size
            height: 40

            border.color: "gray"
            border.width: 1
            radius: 2
            anchors.fill: parent //binding has to be preserved

            Text {
                id: normalText //id only required to preserve binding
                text: control.text //binding has to be preserved
                anchors.fill: parent
                color: "gray"

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }

        Rectangle {
            id: buttonPressed
            color: "#d4d4d4"
            width: 100 //Bit of black magic to define the default size
            height: 40

            border.color: "gray"
            border.width: 1
            radius: 2
            anchors.fill: parent //binding has to be preserved

            Text {
                id: pressedText //id only required to preserve binding
                text: control.text //binding has to be preserved
                anchors.fill: parent
                color: "black"

                horizontalAlignment: Text.AlignHCenter // should not be preserved
                verticalAlignment: Text.AlignVCenter //  should not be preserved
                elide: Text.ElideRight // should not be preserved
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

