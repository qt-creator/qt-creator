import QtQuick 2.6
import QtQuick.Controls 2.0

Slider {
    id: control
    value: 0.5

    background: Item {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: sliderGroove.width
        implicitHeight: sliderGroove.height
        height:  implicitHeight
        width: control.availableWidth

        Rectangle {
            id: sliderGroove
            width: 200
            height: 6
            color: "#bdbebf"
            radius: 2
            anchors.fill: parent
        }

        Item {
            width: control.visualPosition * sliderGroove.width // should be preserved
            height: sliderGrooveLeft.height
            clip: true

            Rectangle {
                id: sliderGrooveLeft
                width: 200
                height: 6
                color: "#21be2b"
                radius: 2
            }
        }

    }

    handle: Item {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2

        implicitWidth: handleNormal.width
        implicitHeight: handleNormal.height
        Rectangle {
            id: handleNormal
            width: 32
            height: 32
            visible: !control.pressed
            color: "#f6f6f6"
            radius: 13
            border.color: "#bdbebf"
        }

        Rectangle {
            id: handlePressed
            width: 32
            height: 32
            visible: control.pressed
            color: "#221bdb"
            radius: 13
            border.color: "#bdbebf"
        }

    }
}