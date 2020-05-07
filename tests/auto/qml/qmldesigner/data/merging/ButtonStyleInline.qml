import QtQuick 2.12


Rectangle {
    id: artboard
    width: 640
    height: 480
    color: "#ee4040"

    Rectangle {

        id: buttonNormal
        x: 286
        y: 62
        color: "#d4d4d4"
        width: 100 //Bit of black magic to define the default size
        height: 60

        border.color: "gray"
        border.width: 1
        radius: 2

        Text {
            id: normalText //id only required to preserve binding
            //binding has to be preserved
            anchors.fill: parent //binding has to be preserved
            color: "gray"
            text: "Normal"

            horizontalAlignment: Text.AlignHCenter
            //luckily enums are interpreted as variant properties and not bindings
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    Rectangle {
        id: buttonPressed
        x: 123
        y: 62
        color: "#69b5ec"
        width: 100 //Bit of black magic to define the default size
        height: 60

        border.color: "gray"
        border.width: 1
        radius: 2

        Text {
            id: pressedText //id only required to preserve binding
            //binding has to be preserved
            anchors.fill: parent
            color: "black"
            text: "pressed"

            horizontalAlignment: Text.AlignHCenter // should not be preserved -
            //luckily enums are interpreted as variant properties and not bindings
            verticalAlignment: Text.AlignVCenter //  should not be preserved
            elide: Text.ElideRight // should not be preserved
        }
    }

    Text {
        id: element
        x: 1
        y: 362
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
