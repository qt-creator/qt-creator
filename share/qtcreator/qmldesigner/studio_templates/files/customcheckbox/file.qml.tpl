import QtQuick 2.12
import QtQuick.Controls 2.12

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
