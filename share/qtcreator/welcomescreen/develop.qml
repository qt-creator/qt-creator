import QtQuick 1.0
import "widgets"
import "components" as Components

Item {
    id: root

    Components.ScrollArea {
        id: scrollArea
        anchors.fill: parent
        Item {
            height: Math.max(recentSessions.height, recentProjects.height)
            width: root.width-40
            RecentSessions {
                id: recentSessions
                x: 10
                width: parent.width / 2 - 10
            }
            RecentProjects {
                id: recentProjects
                x: parent.width / 2 + 10
                width: parent.width / 2 - 10
            }
        }
    }
}


