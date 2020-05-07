import QtQuick 2.10

ListView {
    id: view
    width: listViewBackground.width
    height: listViewBackground.height
    highlight: Rectangle {
        id: listViewHighLight
        width: view.width
        height: 120
        color: "#343434"
        radius: 4
        border.color: "#0d52a4"
        border.width: 8
    }

    highlightMoveDuration: 0

    children: [
        Item {
             z: -1
             anchors.fill: parent

            Rectangle {
                id: listViewBackground
                width: 420
                height: 420
                color: "#69b5ec"
                anchors.fill: parent
            }
        }
    ]

    model: ListModel {
        ListElement {
            name: "Music"
        }
        ListElement {
            name: "Movies"
        }
        ListElement {
            name: "Camera"
        }
        ListElement {
            name: "Map"
        }
        ListElement {
            name: "Calendar"
        }
        ListElement {
            name: "Messaging"
        }
        ListElement {
            name: "Todo List"
        }
        ListElement {
            name: "Contacts"
        }
        ListElement {
            name: "Settings"
        }
    }


    delegate: Item {
        id: delegate
        width: ListView.view.width
        height: delegateNormal.height

        Rectangle {
            id: delegateNormal
            width: 420
            height: 120
            visible: true
            color: "#bdbdbd"
            radius: 4
            anchors.fill: parent
            anchors.margins: 12
            Text {
                id: labelNormal
                color: "#343434"
                text: name
                anchors.top: parent.top
                anchors.margins: 24
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }


        Rectangle {
            id: delegateHighlighted
            width: 420
            height: 120
            visible: false
            color: "#8125eb29"
            radius: 4
            anchors.fill: parent
            anchors.margins: 12
            Text {
                id: labelHighlighted
                color: "#efefef"
                text: name
                anchors.top: parent.top
                anchors.margins: 52
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: delegate.ListView.view.currentIndex = index
        }


        states: [
            State {
                name: "Highlighted"

                when: delegate.ListView.isCurrentItem
                PropertyChanges {
                    target: delegateHighlighted
                    visible: true
                }

                PropertyChanges {
                    target: delegateNormal
                    visible: false
                }


            }
        ]
    }

}