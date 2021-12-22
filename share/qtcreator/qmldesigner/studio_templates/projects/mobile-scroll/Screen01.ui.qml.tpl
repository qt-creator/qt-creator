import QtQuick %{QtQuickVersion}
import QtQuick.Controls %{QtQuickVersion}
import %{ImportModuleName} %{ImportModuleVersion}

Rectangle {
    width: Constants.width
    height: Constants.height

    color: Constants.backgroundColor

     ScrollView {
        anchors.fill: parent

        ListView {
            width: parent.width
            model: 20
            delegate: ItemDelegate {
                text: "Item " + (index + 1)
                width: parent.width
                font.family: Constants.largeFont.family
                font.pixelSize: Constants.largeFont.pixelSize
            }
        }
    }
}
