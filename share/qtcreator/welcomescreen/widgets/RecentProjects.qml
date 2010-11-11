import QtQuick 1.0
import "../components" as Components

HeaderItemView {
    header: qsTr("Recently Edited Projects")
    model: projectList
    delegate: Item {
        Components.QStyleItem { id: styleItem; cursor: "pointinghandcursor"; anchors.fill: parent }
        height: nameText.font.pixelSize*2.5
        width: dataSection.width
        Image{
            id: arrowImage;
            source: "qrc:welcome/images/list_bullet_arrow.png";
            anchors.verticalCenter: parent.verticalCenter;
            anchors.left: parent.left
        }

        Text {
            id: nameText
            text: displayName
            font.bold: true
            width: parent.width
            anchors.top: parent.top
            anchors.left: arrowImage.right
            anchors.leftMargin: 10
        }

        Text {
            text: prettyFilePath
            elide: Text.ElideMiddle
            color: "grey"
            width: parent.width
            anchors.top: nameText.bottom
            anchors.left: arrowImage.right
            anchors.leftMargin: 10
        }

        Timer { id: timer; interval: 500; onTriggered: styleItem.showToolTip(filePath) }

        MouseArea {
            anchors.fill: parent
            onClicked: projectWelcomePage.requestProject(filePath)
            hoverEnabled: true
            onEntered:timer.start()
            onExited: timer.stop()
        }
    }
}
