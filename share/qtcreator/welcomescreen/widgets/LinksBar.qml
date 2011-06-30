import QtQuick 1.0

Row {
    id: tabBar
    height: 25

    property alias model: tabs.model
    property int tabBarWidth

    Repeater {
        id: tabs
        height: tabBar.height
        model: parent.model
        delegate:
            Item {
            width: tabBarWidth / tabs.count
            height: tabBar.height

            Rectangle {
                width: parent.width; height: 1
                anchors { bottom: parent.bottom; bottomMargin: 1 }
                color: "#acb2c2"
            }
            BorderImage {
                id: tabBackground
                anchors.fill: parent
                border { top: 1; bottom: 1}
                source: "qrc:welcome/images/tab_inactive.png"
            }
            Text {
                id: text
                horizontalAlignment: Qt.AlignHCenter; verticalAlignment: Qt.AlignVCenter
                anchors.fill: parent
                text: model.modelData.title
                elide: Text.ElideRight
                color: "white"
            }
            MouseArea {
                id: mouseArea
                hoverEnabled: true
                anchors.fill: parent
                onClicked: tabBar.current = index
            }
            states: [
                State {
                    id: activeState; when: tabBar.current == index
                    PropertyChanges { target: tabBackground; source:"qrc:welcome/images/tab_active.png" }
                    PropertyChanges { target: text; color: "black" }
                },
                State {
                    id: hoverState; when: mouseArea.containsMouse
                    PropertyChanges { target: tabBackground; source:"qrc:welcome/images/tab_hover.png" }
                }

            ]
        }
    }
}
