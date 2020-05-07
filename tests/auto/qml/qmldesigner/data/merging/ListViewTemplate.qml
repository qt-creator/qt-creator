import QtQuick 2.10

ListView {
    id: view
    width: listViewBackground.width
    height: listViewBackground.height

    highlightMoveDuration: 0

    children: [
        Item {
             z: -1
             anchors.fill: parent

            Rectangle {
                id: listViewBackground
                width: 420
                height: 420

                color: "#d80e0e"
                anchors.fill: parent // hsa to be preserved
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

    highlight: Rectangle {
        id: listViewHighLight
        width: view.width // has to be preserved
        height: 120
        color: "#343434"
        radius: 4
        border.color: "#0d52a4"
        border.width: 8
    }

    delegate: Item {
        id: delegate
        width: ListView.view.width
        height: delegateNormal.height

        Rectangle {
            id: delegateNormal
            color: "#bdbdbd"
            anchors.fill: parent
            height: 140
            anchors.margins: 12
            visible: true
            radius: 4
            Text {
                id: labelNormal //id required for binding preservation
                color: "#343434"
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter

                text: name
                anchors.margins: 24
            }
        }

        Rectangle {
            id: delegateHighlighted
            color: "#36bdbdbd"
            anchors.fill: parent
            anchors.margins: 12
            visible: false
            radius: 4
            Text {
                id: labelHighlighted //id required for binding preservation
                color: "#efefef"
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter

                text: name
                anchors.margins: 32
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
