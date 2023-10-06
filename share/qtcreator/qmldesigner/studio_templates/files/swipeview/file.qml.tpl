import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import %{ApplicationImport}

Item {
    width: Constants.width
    height: Constants.height

    SwipeView {
        id: stackView
        width: 100
        anchors.top: tabBar.bottom
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        currentIndex: tabBar.currentIndex

        onCurrentIndexChanged: tabBar.currentIndex = stackView.currentIndex

        Item {
            Label {
                text: qsTr("Page 1")
                anchors.centerIn: parent
                font: Constants.largeFont
            }
        }

        Item {
            Label {
                text: qsTr("Page 2")
                anchors.centerIn: parent
                font: Constants.largeFont
            }
        }

        Item {
            Label {
                text: qsTr("Page 3")
                anchors.centerIn: parent
                font: Constants.largeFont
            }
        }
    }

    TabBar {
        id: tabBar
        currentIndex: 0
        anchors.top: parent.top
        anchors.right: stackView.right
        anchors.left: stackView.left

        TabButton {
            text: qsTr("Tab 1")
        }

        TabButton {
            text: qsTr("Tab 2")
        }

        TabButton {
            text: qsTr("Tab 3")
        }
    }
}
