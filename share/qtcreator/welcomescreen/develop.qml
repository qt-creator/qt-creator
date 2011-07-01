import QtQuick 1.0
import "widgets" as Widgets
import components 1.0 as Components

Item {
    id: root
    Components.ScrollArea {
        id: scrollArea
        anchors.fill: parent
        frame: false
        Item {
            height: Math.max(recentSessions.height, recentProjects.height)
            width: root.width-20
            Widgets.RecentSessions {
                id: recentSessions
                width: parent.width / 2
            }
            Widgets.RecentProjects {
                id: recentProjects
                x: parent.width / 2
                width: parent.width / 2
            }
        }
    }
}


