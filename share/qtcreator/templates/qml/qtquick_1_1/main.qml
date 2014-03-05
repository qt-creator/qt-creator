import QtQuick 1.1

Rectangle {
    width: 360
    height: 360

    MouseArea {
        anchors.fill: parent
        onClicked: {
            Qt.quit();
        }
    }

    Text {
        anchors.centerIn: parent
        text: "Hello World"
    }
}

