import QtQuick 1.0

GridView {
    width: 200
    height: 200

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

    delegate:  Item {
        height: 50
        x: 5

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
