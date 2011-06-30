import QtQuick 1.0
import "widgets" as Widgets
import components 1.0 as Components

Item {
    id: root

    Components.ScrollArea {
        id: scrollArea
        anchors.fill: parent
        Item {
            height: Math.max(recentSessions.height, recentProjects.height)
            width: root.width-40
            Widgets.RecentSessions {
                id: recentSessions
                x: 10
                width: parent.width / 2 - 10
            }
            Widgets.RecentProjects {
                id: recentProjects
                x: parent.width / 2 + 10
                width: parent.width / 2 - 10
            }
        }
    }
}


