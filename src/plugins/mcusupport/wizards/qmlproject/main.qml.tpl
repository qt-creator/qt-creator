import QtQuick 2.0
import CustomModule

Rectangle {

    Row {
        Image {
            anchors.verticalCenter: parent.verticalCenter
            source: "assets/icon.png"
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: " for MCUs"
        }
    }

    CustomButton {
        anchors.centerIn: parent
        text: qsTr("Hello world!")
        onClicked: BackendObject.toggle()
    }

    BackendObject.onCustomPropertyChanged: Qt.uiLanguage = BackendObject.customProperty ? "en_US" : "nb_NO"
}
