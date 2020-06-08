import QtQuick 2.1

Rectangle {
    id: root
    x: 10;
    y: 10;
    Rectangle {
        id: rectangle0
        x: 10;
        y: 10;
        height: 150
        width: 200
    }
    Rectangle {
        id: rectangle2
        x: 100;
        y: 100;
        anchors.fill: root
    }
    Rectangle {
        id: rectangle3
        x: 140;
        y: 180;
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "black"
            }
            GradientStop {
                position: 1
                color: "white"
            }
        }
        Rectangle {
            id: rectangle4
            x: 10
            y: 20
            width: 200
            height: 50
            Rectangle {
                id: rectangle5
                x: 10
                y: 20
                width: 200
                height: 50
            }
        }
    }
}
