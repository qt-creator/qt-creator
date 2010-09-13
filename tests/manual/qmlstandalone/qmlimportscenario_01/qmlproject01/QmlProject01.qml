import Qt 4.7

Rectangle {
    color: "#ddffdd"

    Image {
        source: "apple.svg"
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }

    Text {
        text: "QmlProject01"
        font.pointSize: 14
    }
}
