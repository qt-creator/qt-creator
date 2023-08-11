import QtQuick 2.0
import CustomModule

Rectangle {

    Row {
        Image {
            anchors.centerIn: parent
            source: "assets/icon.png"
        }
        Text {
            anchors.centerIn: parent
            text: " for MCUs"
        }
    }

    CustomButton {
        anchors.centerIn: parent
        text: qsTrId("hello-world")
        onClicked: BackendObject.toggle()
    }

    BackendObject.onCustomPropertyChanged: Qt.uiLanguage = BackendObject.customProperty ? "en_US" : "nb_NO"
    Component.onCompleted: Qt.uiLanguage = "en_US"
}
