import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow
{
    width: 640
    height: 480
    visible: true
    title: "Hello World"

    ColumnLayout {
        anchors.fill: parent

        Text {
            Layout.fillHeight: true
            Layout.fillWidth: true

            text: "Hallo"
        }

        Button {
            text: "Quit"
            height: 20
            Layout.fillWidth: true

            onClicked: { Qt.quit() }
        }
    }
}
