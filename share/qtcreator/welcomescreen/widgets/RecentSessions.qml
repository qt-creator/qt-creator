import QtQuick 1.0
import "../components" as Components

HeaderItemView {
    header: qsTr("Recently Used Sessions")
    model: sessionList

    delegate: Item {
        height: arrowImage.height
        width: dataSection.width

        function fullSessionName()
        {
            var newSessionName = sessionName
            if (model.currentSession)
                newSessionName = qsTr("%1 (current session)").arg(newSessionName);
            return newSessionName;
        }

        Image{
            id: arrowImage;
            source: "qrc:welcome/images/list_bullet_arrow.png";
            anchors.verticalCenter: parent.verticalCenter;
            anchors.left: parent.left
        }

        Text {
            Components.QStyleItem { id: styleItem; cursor: "pointinghandcursor"; anchors.fill: parent }
            id: fileNameText
            text: parent.fullSessionName()
            font.italic: model.defaultSession
            elide: Text.ElideMiddle
            anchors.left: arrowImage.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 10
        }

        Timer { id: timer; interval: 500; onTriggered: styleItem.showToolTip(sessionName) }

        MouseArea {
            anchors.fill: parent
            onClicked: projectWelcomePage.requestSession(sessionName)
            hoverEnabled: true
            onEntered:timer.start()
            onExited: timer.stop()
        }
    }
}
