import QtQuick 2.1
import QtQuick.Controls 1.0

Rectangle {
    property string title

    property Item toolBar
    property Item statusBar

    property alias contentItem : contentArea
    default property alias data: contentArea.data

    onStatusBarChanged: { if (statusBar) { statusBar.parent = statusBarArea } }
    onToolBarChanged: { if (toolBar) { toolBar.parent = toolBarArea } }

    property int maximumWidth: 0
    property int minimumWidth: 0

    property int maximumHeight: 0
    property int minimumHeight: 0

    Item {
        id: contentArea
        anchors.top: toolBarArea.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: statusBarArea.top
    }

    Item {
        id: toolBarArea
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        implicitHeight: childrenRect.height
        height: visibleChildren.length > 0 ? implicitHeight: 0
    }

    Item {
        id: statusBarArea
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        implicitHeight: childrenRect.height
        height: 0
        //The status bar is not visible for now
        //height: visibleChildren.length > 0 ? implicitHeight: 0
    }

}
