import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

Item {
    id: page
    width: 300
    height: 300
    property alias icon: icon
    property alias topLeftRect: topLeftRect
    property alias bottomLeftRect: bottomLeftRect
    property alias middleRightRect: middleRightRect

    property alias mouseArea2: mouseArea2
    property alias mouseArea1: mouseArea1
    property alias mouseArea: mouseArea

    Image {
        id: icon
        x: 10
        y: 20
        source: "qt-logo.png"
    }

    Rectangle {
        id: topLeftRect
        width: 55
        height: 41
        color: "#00000000"
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.top: parent.top
        anchors.topMargin: 20
        border.color: "#808080"

        MouseArea {
            id: mouseArea
            anchors.fill: parent
        }
    }

    Rectangle {
        id: middleRightRect
        width: 55
        height: 41
        color: "#00000000"
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        border.color: "#808080"
        MouseArea {
            id: mouseArea1
            anchors.fill: parent
        }
    }

    Rectangle {
        id: bottomLeftRect
        width: 55
        height: 41
        color: "#00000000"
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        border.color: "#808080"
        MouseArea {
            id: mouseArea2
            anchors.fill: parent
        }
        anchors.left: parent.left
        anchors.leftMargin: 10
    }
}
