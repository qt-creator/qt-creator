import QtQuick 2.12


Item {
    id: artboard
    width: 640
    height: 480

    Rectangle {
        id: sliderGroove
        x: 78
        y: 127
        width: 200
        height: 6
        color: "#bdbebf"
    }

    Rectangle {
        id: sliderGrooveLeft
        x: 78
        y: 165
        width: 200
        height: 6
        color: "#21be2b"
        radius: 2
    }

    Rectangle {
        id: handleNormal
        x: 130
        y: 74
        width: 32
        height: 32
        radius: 13
        color: "#f6f6f6"
        border.color: "#bdbebf"
    }
    Rectangle {
        id: handlePressed
        x: 78
        y: 74
        width: 32
        height: 32
        radius: 13
        color:  "#221bdb"
        border.color: "#bdbebf"
    }

    Text {
        id: element
        x: 8
        y: 320
        color: "#eaeaea"
        text: qsTrId("Some stuff for reference that is thrown away")
        font.pixelSize: 32
    }


}

/*##^##
Designer {
    D{i:0;formeditorColor:"#000000"}
}
##^##*/
