import QtQuick 2.12


Rectangle {
    id: artboard
    width: 800
    height: 600
    color: "#ee4040"

    Rectangle {
        id: listViewBackground
        x: 19
        y: 34
        width: 420
        height: 420

        color: "#69b5ec"

    }

    Rectangle {
        id: delegateNormal
        x: 19
        y: 51
        color: "#bdbdbd"

        height: 120

        width: 420

        radius: 4
        Text {
            id: labelNormal //id required for binding preservation
            color: "#343434"
            text: "some text"
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter

            anchors.margins: 24
        }
    }


    Rectangle {
        id: delegateHighlighted
        x: 19
        y: 177
        color: "#8125eb29"
        height: 120

        width: 420

        radius: 4
        Text {
            id: labelHighlighted //id required for binding preservation
            color: "#efefef"
            text: "some text"
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter

            anchors.margins: 52
        }
    }


    Rectangle {
        id: listViewHighLight
        x: 19
        y: 323
        width: 420
        height: 120
        color: "#343434"
        radius: 4
        border.color: "#0d52a4"
        border.width: 8
    }



}


