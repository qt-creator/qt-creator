import QtQuick 2.15
import Constants 1.0

Rectangle {
    width: Constants.width
    height: Constants.height
    color: Constants.backgroundColor

    Text {
        text: qsTr("Hello %{ProjectName}")
        anchors.centerIn: parent
    }
}
