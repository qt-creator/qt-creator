import Qt 4.6

Rectangle {
    x: 640
    y: 480
    Component {
        id: redSquare
        Rectangle {
            color: "red"
            width: 100
            height: 100
        }
    }

    Loader { sourceComponent: redSquare;}
    Loader { sourceComponent: redSquare; x: 20 }
}
