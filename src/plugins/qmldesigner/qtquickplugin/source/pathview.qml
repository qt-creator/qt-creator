import QtQuick 1.0

PathView {
    width: 250
    height: 200

    path: Path {
        startX: 120
        startY: 100
        PathQuad { x: 120; y: 25; controlX: 260; controlY: 75 }
        PathQuad { x: 120; y: 100; controlX: -20; controlY: 75 }
    }
    model: ListModel {
        ListElement {
            name: "Grey"
            colorCode: "grey"
        }
        ListElement {
            name: "Red"
            colorCode: "red"
        }
        ListElement {
            name: "Blue"
            colorCode: "blue"
        }
        ListElement {
            name: "Green"
            colorCode: "green"
        }
    }
    delegate: Component {
        Column {
            spacing: 5
            Rectangle {
                width: 40
                height: 40
                color: colorCode
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Text {
                x: 5
                text: name
                anchors.horizontalCenter: parent.horizontalCenter
                font.bold: true
            }
        }
    }
}

