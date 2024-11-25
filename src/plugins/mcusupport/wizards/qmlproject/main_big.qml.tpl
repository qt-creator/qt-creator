import QtQuick 2.0
import CustomModule

Rectangle {
     // Component.onCompleted: Qt.uiLanguage = "nb_NO" // Uncomment to change the UI language //TODO: Is this the "official" method of setting ui language?

    Row {
        visible: CustomObject.customProperty
        Image {
            anchors.verticalCenter: parent.verticalCenter
            id: icon
            source: "assets/icon.png"
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: 28
            id: title
            text: " for MCUs"
        }
    }

    CustomComponent {
        anchors.centerIn: parent
    }

    MouseArea {
        anchors.fill: parent
        onClicked: CustomObject.toggle()
    }
}
