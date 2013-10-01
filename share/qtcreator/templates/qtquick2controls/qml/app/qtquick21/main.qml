import QtQuick 2.1
import QtQuick.Controls 1.0

ApplicationWindow {
    title: qsTr("Hello World")
    width: 640
    height: 480

    menuBar: MenuBar {
        Menu {
            title: qsTr("File")
            MenuItem {
                text: qsTr("Exit")
                onTriggered: Qt.quit();
            }
        }
    }

    Button {
        text: qsTr("Hello World")
        anchors.centerIn: parent
    }
}
