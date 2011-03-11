import QtQuick 1.1
import Monitor 1.0
import "MainView.js" as Plotter

Rectangle {
    property string label
    signal clicked

    anchors.verticalCenter: parent.verticalCenter
    anchors.verticalCenterOffset: -1
    width: 30; height: 22

    border.color: "#cc80b2f6"
    color: "transparent"

    Text {
        anchors.centerIn: parent
        text: label
        color: "white"
        font.pixelSize: 14
    }

    MouseArea {
        anchors.fill: parent
        onClicked: parent.clicked()
    }
}
