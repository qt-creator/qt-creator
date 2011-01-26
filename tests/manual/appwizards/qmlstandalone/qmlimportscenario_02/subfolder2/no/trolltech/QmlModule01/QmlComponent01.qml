import Qt 4.7

Rectangle {
    color: "#ddffdd"

    Image {
        source: "apple.svg"
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }

    Text {
        text: "QmlComponent01"
        font.pointSize: 14
    }
}
