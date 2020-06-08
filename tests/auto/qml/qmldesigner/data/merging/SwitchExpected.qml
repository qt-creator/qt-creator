import QtQuick 2.10
import QtQuick.Templates 2.1 as T
import TemplateMerging 1.0

T.Switch {
    id: control

    implicitWidth: background.implicitWidth
    implicitHeight: background.implicitHeight

    text: "test"
    indicator: Rectangle {
        id: switchIndicator
        x: control.leftPadding
        y: 34
        width: 64
        height: 44
        color: "#e9e9e9"
        radius: 16
        border.color: "#dddddd"
        anchors.verticalCenter: parent.verticalCenter
        Rectangle {
            id: switchHandle
            width: 31
            height: 44
            color: "#e9e9e9"
            radius: 16
            border.color: "#808080"
        }
    }

    background: Item {
        implicitWidth: switchBackground.width
        implicitHeight: switchBackground.height

        Rectangle {
            id: switchBackground
            width: 144
            height: 52
            color: "#c2c2c2"
            border.color: "#808080"
            anchors.fill: parent
            Text {
                id: switchBackgroundText
                text: control.text
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 12
            }
        }
    }

    leftPadding: 4

    contentItem: Item  { //designer want to edit the label as part of background
    }


    states: [
        State {
            name: "off"
            when: !control.checked && !control.down
        },
        State {
            name: "on"
            when: control.checked && !control.down

            PropertyChanges {
                target: switchIndicator
                color: "#1713de"
                border.color: "#1713de"
            }

            PropertyChanges {
                target: switchHandle
                x: parent.width - width
            }
        },
        State {
            name: "off_down"
            when: !control.checked && control.down

            PropertyChanges {
                target: switchIndicator
                color: "#e9e9e9"
            }

            PropertyChanges {
                target: switchHandle
                color: "#d2d2d2"
                border.color: "#d2d2d2"
            }
        },
        State {
            name: "on_down"
            when: control.checked && control.down

            PropertyChanges {
                target: switchHandle
                x: parent.width - width
                color: "#e9e9e9"
            }

            PropertyChanges {
                target: switchIndicator
                color: "#030381"
                border.color: "#030381"
            }
        }
    ]
}