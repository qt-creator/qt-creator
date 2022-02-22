/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/

import QtQuick 2.15
import QtQuick.Controls 2.15

Switch {
    id: control

    implicitWidth: backgroundItem.implicitWidth
    implicitHeight: backgroundItem.implicitHeight

    readonly property int baseSize: 12

    background: backgroundItem
    Rectangle {
        id: backgroundItem
        color: "#00000000"
        implicitWidth: control.baseSize * 6.0
        implicitHeight: control.baseSize * 3.8
    }

    leftPadding: 4

    indicator: switchHandle
    Rectangle {
        id: switchHandle
        implicitWidth: control.baseSize * 4.8
        implicitHeight: control.baseSize * 2.6
        x: control.leftPadding
        color: "#e9e9e9"
        anchors.verticalCenter: parent.verticalCenter
        radius: control.baseSize * 1.3

        Rectangle {
            id: rectangle

            width: control.baseSize * 2.6
            height: control.baseSize * 2.6
            radius: control.baseSize * 1.3
            color: "#e9e9e9"
        }
    }
    states: [
        State {
            name: "off"
            when: !control.checked && !control.down

            PropertyChanges {
                target: rectangle
                color: "#cccccc"
            }

            PropertyChanges {
                target: switchHandle
                color: "#00000000"
                border.color: "#aeaeae"
            }
        },
        State {
            name: "on"
            when: control.checked && !control.down

            PropertyChanges {
                target: switchHandle
                color: "#047eff"
                border.color: "#ffffff"
            }

            PropertyChanges {
                target: rectangle
                x: parent.width - width
            }
        },
        State {
            name: "off_down"
            when: !control.checked && control.down

            PropertyChanges {
                target: rectangle
                color: "#e9e9e9"
            }

            PropertyChanges {
                target: switchHandle
                color: "#00000000"
                border.color: "#047eff"
            }
        },
        State {
            name: "on_down"
            when: control.checked && control.down

            PropertyChanges {
                target: rectangle
                x: parent.width - width
                color: "#e9e9e9"
            }

            PropertyChanges {
                target: switchHandle
                color: "#b1047eff"
                border.color: "#ffffff"
            }
        }
    ]
}
