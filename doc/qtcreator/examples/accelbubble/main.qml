import QtQuick
import QtSensors

Window {
    id: mainWindow
    width: 320
    height: 480
    visible: true
    title: qsTr("Accelerate Bubble")
    readonly property double radians_to_degrees: 180 / Math.PI

    Accelerometer {
        id: accel
        dataRate: 100
        active:true

        onReadingChanged: {
            var newX = (bubble.x + calcRoll(accel.reading.x, accel.reading.y, accel.reading.z) * .1)
            var newY = (bubble.y - calcPitch(accel.reading.x, accel.reading.y, accel.reading.z) * .1)

            if (isNaN(newX) || isNaN(newY))
                return;

            if (newX < 0)
                newX = 0

            if (newX > mainWindow.width - bubble.width)
                newX = mainWindow.width - bubble.width

            if (newY < 18)
                newY = 18

            if (newY > mainWindow.height - bubble.height)
                newY = mainWindow.height - bubble.height

                bubble.x = newX
                bubble.y = newY
        }
    }

    function calcPitch(x,y,z) {
        return -Math.atan2(y, Math.hypot(x, z)) * mainWindow.radians_to_degrees;
    }
    function calcRoll(x,y,z) {
        return -Math.atan2(x, Math.hypot(y, z)) * mainWindow.radians_to_degrees;
    }

    Image {
        id: bubble
        source: "Bluebubble.svg"
        smooth: true
        property real centerX: mainWindow.width / 2
        property real centerY: mainWindow.height / 2
        property real bubbleCenter: bubble.width / 2
        x: centerX - bubbleCenter
        y: centerY - bubbleCenter

        Behavior on y {
            SmoothedAnimation {
                easing.type: Easing.Linear
                duration: 100
            }
        }
        Behavior on x {
            SmoothedAnimation {
                easing.type: Easing.Linear
                duration: 100
            }
        }
    }
}
