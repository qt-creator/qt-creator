/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/

import QtQuick 2.15
import QtQuick.Controls 2.15

CheckBox {
    id: control
    text: qsTr("CheckBox")
    checked: true

    indicator: indicatorRectangle
    Rectangle {
        id: indicatorRectangle
        implicitWidth: 26
        implicitHeight: 26
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: 3
        border.color: "#047eff"

        Rectangle {
            id: rectangle
            width: 14
            height: 14
            x: 6
            y: 6
            radius: 2
            visible: false
            color: "#047eff"
        }
    }

    contentItem: textItem
    Text {
        id: textItem
        text: control.text
        font: control.font
        opacity: enabled ? 1.0 : 0.3
        color: "#047eff"
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + control.spacing
    }
    states: [
        State {
            name: "checked"
            when: control.checked

            PropertyChanges {
                target: rectangle
                visible: true
                color: "#047eff"
            }

            PropertyChanges {
                target: indicatorRectangle
                color: "#00000000"
                border.color: "#047eff"
            }

            PropertyChanges {
                target: textItem
                color: "#047eff"
            }
        },
        State {
            name: "unchecked"
            when: !control.checked

            PropertyChanges {
                target: rectangle
                visible: false
            }

            PropertyChanges {
                target: indicatorRectangle
                color: "#00000000"
                border.color: "#047eff"
            }

            PropertyChanges {
                target: textItem
                color: "#047eff"
            }
        }
    ]
}
