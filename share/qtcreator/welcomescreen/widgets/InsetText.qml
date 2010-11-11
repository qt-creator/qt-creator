import QtQuick 1.0

Text {
    property color mainColor: "darkgrey"
    Text {
        x: 0; y: -1
        text: parent.text
        color: parent.mainColor
        font.bold: parent.font.bold
        font.pointSize: parent.font.pointSize
        font.italic: parent.font.italic
    }
    text: "Featured News"
    color: "white"
}
