import Qt 4.7
import "widgets"

Image {
    id: root
    source: "qrc:welcome/images/welcomebg.png"
    smooth: true

    // work around the fact that we can't use
    // a property alias to welcomeMode.activePlugin
    property int current: 0
    onCurrentChanged: welcomeMode.activePlugin = current
    Component.onCompleted: current = welcomeMode.activePlugin

    BorderImage {
        id: inner_background
        Image {
            id: header;
            source: "qrc:welcome/images/center_frame_header.png";
            anchors.verticalCenter: parent.verticalCenter;
            anchors.horizontalCenter: parent.horizontalCenter;
            anchors.topMargin: 2
        }
        anchors.top: root.top
        source: "qrc:welcome/images/background_center_frame_v2.png"
        width: parent.width
        height: 60
        border.right: 2
        border.left: 2
        border.top: 2
        border.bottom: 10
    }

    LinksBar {
        id: navigationAndDevLinks
        property alias current: root.current
        anchors.top: inner_background.bottom
        anchors.left: parent.left
        anchors.bottomMargin: 4
        anchors.topMargin: -2
        width: parent.width
        model: tabs.model
        tabBarWidth: width
    }

    BorderImage {
        id: news
        opacity: 0.7
        source: "qrc:welcome/images/rc_combined.png"
        border.left: 5; border.top: 5
        border.right: 5; border.bottom: 5
        anchors.top: navigationAndDevLinks.bottom
        anchors.bottom: feedback.top
        anchors.left: parent.left
        anchors.margins: 5
        width: 270
        FeaturedAndNewsListing {
            anchors.fill: parent
            anchors.margins: 4
        }
    }

    TabWidget {
        id: tabs
        property int current: root.current
        model: pagesModel
        anchors.top: navigationAndDevLinks.bottom
        anchors.bottom: feedback.top
        anchors.left: news.right
        anchors.right: parent.right
        anchors.leftMargin: 0
        anchors.margins: 4
    }

    Feedback {
        id: feedback
        anchors.bottom: parent.bottom
        width: parent.width
    }

}
