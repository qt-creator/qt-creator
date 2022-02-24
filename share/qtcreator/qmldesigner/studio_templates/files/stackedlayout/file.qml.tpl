import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import %{ApplicationImport}

Item {
    width: Constants.width
    height: Constants.height

    StackLayout {
        id: stackLayout
        width: 100
        anchors.top: tabBar.bottom
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        currentIndex: tabBar.currentIndex

        Item {
            Label {
                text: qsTr("Page 01")
                anchors.centerIn: parent
                font: Constants.largeFont
            }
        }

        Item {
            Label {
                text: qsTr("Page 02")
                anchors.centerIn: parent
                font: Constants.largeFont
            }
        }

        Item {
            Label {
                text: qsTr("Page 03")
                anchors.centerIn: parent
                font: Constants.largeFont
            }
        }
    }

    TabBar {
        id: tabBar
        currentIndex: 0
        anchors.top: parent.top
        anchors.right: stackLayout.right
        anchors.left: stackLayout.left

        TabButton {
            text: qsTr("Tab 0")
        }

        TabButton {
            text: qsTr("Tab 1")
        }

        TabButton {
            text: qsTr("Tab 2")
        }
    }
}
