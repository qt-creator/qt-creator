import QtQuick 2.12

Item {
    width: 640
    height: 480

    Rectangle {
        id: switchIndicator
        x: 219
        y: 34
        width: 64
        height: 44

        color: "#e9e9e9"

        radius: 16
        border.color: "#dddddd"

        Rectangle {
            id: switchHandle //id is required for states

            width: 31
            height: 44
            radius: 16
            color: "#e9e9e9"
            border.color: "#808080"
        }
    }

    Rectangle {
        id: switchBackground
        x: 346
        y: 27
        width: 144
        height: 52
        color: "#c2c2c2"
        border.color: "#808080"

        Text {
            id: switchBackgroundText
            text: "background"
            anchors.right: parent.right

            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 12
        }
    }

    Text {
        id: element
        x: 1
        y: 362
        color: "#eaeaea"
        text: qsTrId("Some stuff for reference that is thrown away")
        font.pixelSize: 32
    }

    Rectangle { //This is ignored when merging
        id: weirdStuff02
        x: 8
        y: 87
        width: 624
        height: 200
        color: "#ffffff"
    }
}
