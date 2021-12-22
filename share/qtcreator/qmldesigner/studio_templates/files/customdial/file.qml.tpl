import QtQuick 2.12
import QtQuick.Controls 2.12

Dial {
    id: control
    property alias controlEnabled: control.enabled
    background: backgroundRect
    Rectangle {
        id: backgroundRect
        x: control.width / 2 - width / 2
        y: control.height / 2 - height / 2
        implicitWidth: Math.max(64, Math.min(control.width, control.height))
        implicitHeight: width
        color: "transparent"
        radius: control.width / 2
        border.color: "#047eff"
        border.width: 2
    }

    handle: Item {
        id: handleParent
        x: control.background.x + control.background.width / 2 - width / 2
        y: control.background.y + control.background.height / 2 - height / 2
        transform: [
            Translate {
                y: -Math.min(
                       control.background.width,
                       control.background.height) * 0.4 + handleParent.height / 2
            },
            Rotation {
                angle: control.angle
                origin.x: handleParent.width / 2
                origin.y: handleParent.height / 2
            }
        ]
        width: handleItem.width
        height: handleItem.height
    }

    Rectangle {
        id: handleItem
        parent: handleParent

        width: 16
        height: 16
        color: "#047eff"
        radius: 8
        border.color: "#047eff"
        antialiasing: true
    }
    states: [
        State {
            name: "normal"
            when: !control.pressed && control.enabled

            PropertyChanges {
                target: handleItem
                color: "#047eff"
            }
        },
        State {
            name: "disabled"
            when: !control.enabled

            PropertyChanges {
                target: handleItem
                opacity: 1
                color: "#7f047eff"
                border.color: "#00000000"
            }

            PropertyChanges {
                target: background
                opacity: 0.3
            }

            PropertyChanges {
                target: backgroundRect
                border.color: "#7f047eff"
            }
        },
        State {
            name: "pressed"
            when: control.pressed

            PropertyChanges {
                target: handleItem
                color: "#047eff"
                border.color: "#ffffff"
            }

            PropertyChanges {
                target: backgroundRect
                border.color: "#047eff"
            }
        }
    ]
}
