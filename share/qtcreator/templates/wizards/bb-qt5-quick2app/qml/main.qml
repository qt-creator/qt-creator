import QtQuick 2.0

Rectangle {
    width: 1024
    height: 600
    Text {
        text: qsTr("Hello from QtQuick2")
        anchors.centerIn: parent
    }
    MouseArea {
        anchors.fill: parent
        onClicked: {
            Qt.quit();
        }
    }
}

