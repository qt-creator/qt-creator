import QtQuick 1.1

Rectangle {
    id: button

    property alias text: textItem.text
    property bool recording: false

    signal clicked

    width: 80; height: textItem.height + 8
    border.width: 1
    border.color: Qt.darker(button.color, 1.4)
    radius: height/2
    smooth: true

    color: "#049e0e"

    Text {
        id: textItem
        anchors.centerIn: parent
        font.pixelSize: 14
        color: "white"
        text: "Recording"
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: {
            recording = !recording;
            button.clicked()
        }
    }

    StateGroup {
        id: recordState
        states: State {
            name: "recording"
            when: recording
            PropertyChanges { target: button; color: "#ff120c"; }
            PropertyChanges { target: textItem; text: "Stop" }
        }
    }

    StateGroup {
        states: State {
            name: "pressed"; when: mouseArea.pressed && mouseArea.containsMouse
            PropertyChanges { target: button; color: Qt.darker(color, 1.1); explicit: true }
            PropertyChanges { target: textItem; x: textItem.x + 1; y: textItem.y + 1; explicit: true }
        }
    }
}
