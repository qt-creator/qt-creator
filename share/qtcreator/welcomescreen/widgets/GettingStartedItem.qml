import QtQuick 1.1
import qtcomponents 1.0

Item {
    id: gettingStartedItem
    width: 200
    height: 300

    property alias title: titleText.text
    property alias description: descriptionText.text
    property int number: 1
    property alias imageUrl: image.source

    signal clicked

    CustomColors {
        id: colors
    }

    CustomFonts {
        id: fonts
    }

    QStyleItem { cursor: "pointinghandcursor"; anchors.fill: parent }

    Rectangle {
        y: 170
        width: 20
        height: 20
        color: "#7383a7"
        radius: 20
        anchors.left: parent.left
        anchors.leftMargin: 8
        smooth: true
        visible: false

        Text {
            color: "#f7f7f7"
            font.bold: true
            text: gettingStartedItem.number
            font.family: "Helvetica"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: 15
        }
    }

    Text {
        id: titleText
        y: 188
        color: colors.strongForegroundColor
        wrapMode: Text.WordWrap
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.right: parent.right
        anchors.rightMargin: 8
        font: fonts.standardCaption
    }

    Text {
        id: descriptionText
        y: 246
        height: 62
        color: "#828282"
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 2
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.left: parent.left
        anchors.leftMargin: 8
        horizontalAlignment: Text.AlignJustify
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        font.bold: false
        font.family: "Helvetica"
    }

    Item {
        id: item1
        y: 22
        width: 184
        height: 160
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.left: parent.left
        anchors.leftMargin: 8

        Image {
            id: image
            x: 0
            y: 0
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    Rectangle {
        id: border
        color: "#00000000"
        radius: 8
        visible: false
        anchors.fill: parent
        border.color: "#dddcdc"
        anchors.margins: 4
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onEntered: {
            gettingStartedItem.state="hover"
        }

        onExited: {
            gettingStartedItem.state=""
        }

        onClicked: {
            gettingStartedItem.clicked();
        }

    }

    states: [
        State {
            name: "hover"

            PropertyChanges {
                target: border
                visible: true
            }
        }
    ]
}
