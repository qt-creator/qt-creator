import QtQuick 1.1

Item {
    property alias text: txt.text

    height: 50
    width: 150    //### required, or ignored by positioner

    Text {
        id: txt;
        x: 5
        font.pixelSize: 12
        color: "#232323"
        anchors.verticalCenter: parent.verticalCenter
    }

    Rectangle {
        height: 1
        width: parent.width
        color: "#cccccc"
        anchors.bottom: parent.bottom
    }
}
