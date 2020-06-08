import QtQuick 2.1

Rectangle {
    id: root
    x: 10;
    y: 10;
    Rectangle {
        id: rectangle0
        x: 10
        y: 10
        width: 200
        height: 150

        Image {
            id: rectangle1
            x: 10
            y: 10
            width: 100
            height: 150
            source: "qt/icon.png"
        }
    }

    Rectangle {
        id: rectangle2
        x: 100;
        y: 100;
        anchors.fill: root
    }

    Rectangle {
        id: rectangle3
        x: 140
        y: 180
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#ffffff"
            }

            GradientStop {
                position: 1
                color: "#000000"
            }
        }
        Image {
            id: rectangle4
            x: 10
            y: 20
            width: 100
            height: 150
            source: "qt/realcool.jpg"
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
