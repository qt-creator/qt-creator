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
            height: 4

            anchors.fill: parent // has to be preserved
            radius: 2
            color: "#bdbebf"
        }

        Item {
            width: control.visualPosition * sliderGroove.width // should be preserved
            height: sliderGrooveLeft.height
            clip: true

            Rectangle {
                id: sliderGrooveLeft
                width: 200
                height: 4
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
            width: 26
            height: 26
            radius: 13
            color: "#f6f6f6"
            visible: !control.pressed //has to be preserved
            border.color: "#bdbebf"
        }
        Rectangle {
            id: handlePressed
            width: 26
            height: 26
            radius: 13
            visible: control.pressed //has to be preserved
            color:  "#f0f0f0"
            border.color: "#bdbebf"
        }
    }
}
