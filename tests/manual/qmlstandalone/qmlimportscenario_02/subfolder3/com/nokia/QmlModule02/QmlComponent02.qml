import Qt 4.7

Rectangle {
    color: "#ffdddd"

    Image {
        source: "tomato.svg"
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }

    Text {
        text: "QmlComponent02"
        font.pointSize: 14
    }
}
