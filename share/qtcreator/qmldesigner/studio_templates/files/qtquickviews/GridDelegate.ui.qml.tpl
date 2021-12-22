import QtQuick 2.15

Item {
    id: delegate
    width: 140
    height: 140

    Rectangle {
        id: rectangle
        color: "#bdbdbd"
        anchors.fill: parent
        anchors.margins: 12
        visible: true
        radius: 4
    }

    Text {
        id: label
        color: "#343434"
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter

        font.family: Constants.largeFont.family
        text: name
        anchors.margins: 24
    }
    MouseArea {
        anchors.fill: parent
        onClicked: delegate.GridView.view.currentIndex = index
    }
    states: [
        State {
            name: "Highlighted"

            when: delegate.GridView.isCurrentItem
            PropertyChanges {
                target: label
                color: "#efefef"
                anchors.topMargin: 52
            }

            PropertyChanges {
                target: rectangle
                visible: false
            }
        }
    ]
}
