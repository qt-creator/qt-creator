import QtQuick 2.14
import QtQuick.Window 2.14
import QtSensors 5.12

Window {
    id: window
    visible: true
    property alias mainWindow: mainWindow
    property alias bubble: bubble
    Rectangle {
        id: mainWindow
        color: "#ffffff"
        anchors.fill: parent

        Bubble {
            id: bubble
            x: bubble.centerX - bubbleCenter
            y: bubble.centerY - bubbleCenter
            bubbleCenter: bubble.width /2
            centerX: mainWindow.width /2
            centerY: mainWindow.height /2

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

    Accelerometer {
       id: accel
       dataRate: 100
       active: true
       readonly property double radians_to_degrees: 180 / Math.PI

       onReadingChanged: {
           var newX = (bubble.x + calcRoll(accel.reading.x, accel.reading.y, accel.reading.z) * 0.1)
           var newY = (bubble.y - calcPitch(accel.reading.x, accel.reading.y, accel.reading.z) * 0.1)

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
        return -Math.atan2(y, Math.hypot(x, z)) * accel.radians_to_degrees;
    }
    function calcRoll(x,y,z) {
        return -Math.atan2(x, Math.hypot(y, z)) * accel.radians_to_degrees;
    }
}
