import QtQuick %{QtQuickVersion}
import %{ImportModuleName} %{ImportModuleVersion}

Rectangle {
    width: Constants.width
    height: Constants.height

    color: Constants.backgroundColor

    property alias title: textItem.text

    Text {
        id: textItem
        text: qsTr("Hello %{ProjectName}") + " 02"
        anchors.centerIn: parent
        font.family: Constants.largeFont.family
        font.pixelSize: Constants.largeFont.pixelSize
    }
}
