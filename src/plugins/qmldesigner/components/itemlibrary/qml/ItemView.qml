import Qt 4.6

Item {
    id: itemView

    property var style

    width: GridView.view.cellWidth
    height: style.cellHeight

    signal itemClicked()
    signal itemDragged()

    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right        
        height: 1
        color: style.gridLineLighter
    }
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: style.gridLineDarker
    }
    Rectangle {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: 1
        color: style.gridLineLighter
    }
    Rectangle {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 1
        color: style.gridLineDarker
    }
    Rectangle {
        anchors.top:parent.top
        anchors.right:parent.right
        width:1
        height:1
        color: style.backgroundColor
    }
    Rectangle {
        anchors.bottom:parent.bottom
        anchors.left:parent.left
        width:1
        height:1
        color: style.backgroundColor
    }

    Image {
        id: itemIcon

        anchors.top: parent.top
        anchors.topMargin: style.cellMargin
        anchors.horizontalCenter: parent.horizontalCenter

        width: itemLibraryIconWidth
        height: itemLibraryIconHeight
        pixmap: itemPixmap
    }

    Text {
        id: text

        anchors.top: itemIcon.bottom
        anchors.topMargin: itemView.style.cellSpacing
        anchors.horizontalCenter: parent.horizontalCenter
        width: style.textWidth
        height: style.textHeight

        verticalAlignment: "AlignVCenter"
        horizontalAlignment: "AlignHCenter"
        text: itemName
        // workaround: text color not updated when 'style' var finally assigned
        color: style.itemNameTextColor
        Component.onCompleted: text.color = style.itemNameTextColor
    }

    MouseRegion {
        id: mouseRegion
        anchors.fill: parent

        onPositionChanged: {
            itemDragged();
        }
        onClicked: {
            itemClicked();
        }
    }
}

