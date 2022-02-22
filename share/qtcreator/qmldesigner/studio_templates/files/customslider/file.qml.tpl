/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/

import QtQuick 2.15
import QtQuick.Controls 2.15

Slider {
    id: control
    value: 0.5

    background: backgroundRect

    Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 200
        implicitHeight: 4
        parent: control.background
        id: backgroundRect

        width: control.availableWidth
        height: implicitHeight
        radius: 2
        color: "#bdbebf"

        Rectangle {
            id: groove
            height: parent.height
            color: "#047eff"
            radius: 2
            width: control.visualPosition * parent.width
        }
    }

    handle: Item {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        height: handleItem.height
        width: handleItem.width
    }

    Rectangle {
        parent: control.handle
        id: handleItem
        implicitWidth: 26
        implicitHeight: 26
        radius: 13
        color: "#f6f6f6"
        border.color: "#bdbebf"
    }
    states: [
        State {
            name: "normal"
            when: !control.pressed

            PropertyChanges {
                target: groove
                color: "#047eff"
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
                target: groove
                color: "#047eff"
            }
        }
    ]
}
