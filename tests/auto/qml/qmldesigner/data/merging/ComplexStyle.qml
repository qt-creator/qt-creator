import QtQuick 2.1

Item {
    Rectangle {
        id: rectangle0
        Image {
            id: rectangle1
            x: 10
            y: 10
            height: 150
            width: 100
            source: "qt/icon.png"
        }
    }
    Rectangle {
        id: rectangle3
        x: 140;
        y: 180;
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "white"
            }

            GradientStop {
                position: 1
                color: "black"
            }
        }
    }
    Rectangle {
        id: rectangle5
        x: 160
        y: 220
        width: 200
        height: 50
    }
    Image {
        id: rectangle4
        x: 150;
        y: 200;
        height: 150
        width: 100
        source: "qt/realcool.jpg"
    }
}
