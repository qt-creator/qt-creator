import Qt 4.7

Rectangle {
    property string text: "test"
    property real myNumber: 10
    default property alias content: text.children
    width: 200
    height: 200
    Text {
        id: text
        anchors.fill: parent
    }

}
