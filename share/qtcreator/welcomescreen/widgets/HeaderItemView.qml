import QtQuick 1.0

Item {
    id: root
    height: childrenRect.height
    property string header
    property QtObject model
    property Component delegate

    Text {
        id: titleText
        text: root.header
        font.bold: true
        font.pointSize: 14
        color: "#555555"
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
    }

    Column {
        id: dataSection
        spacing: 10
        anchors.topMargin: 10
        anchors.top: titleText.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        Repeater {
            model: root.model
            delegate: root.delegate
        }
    }
}
