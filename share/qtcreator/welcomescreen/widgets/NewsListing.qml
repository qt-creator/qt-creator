import Qt 4.7
import "../components/" as Components

Item {
    id: root

    property int currentItem: 0
    property alias model: repeater.model
    property alias itemCount: repeater.count

    Timer {
        id: timer
        repeat: true
        interval: 30*1000
        onTriggered: repeater.incrementIndex()
    }

    Repeater {
        id: repeater
        function incrementIndex() {
            repeater.itemAt(currentItem).active = false
            currentItem = (currentItem+1) % repeater.count
            repeater.itemAt(currentItem).active = true
        }

        function handleModelChanged() {
            if (timer.running)
                timer.stop();
            currentItem = 0
            //FIXME: this doesn't work
            repeater.itemAt(currentItem).active = true
            timer.start()
        }

        anchors.fill: parent
        onModelChanged: handleModelChanged()
        delegate: Item {
            property bool active: false
            id: delegateItem
            opacity: 0
            height: root.height
            width: 270
            Column {
                spacing: 10
                width: parent.width
                id: column
                Text { id: heading1; text: title; font.bold: true; wrapMode: Text.WrapAtWordBoundaryOrAnywhere; textFormat: Text.RichText; width: parent.width-icon.width-5 }
                Row {
                    spacing: 5
                    width: parent.width
                    Image { id: icon; source: blogIcon; asynchronous: true }
                    Text { id: heading2; text: blogName; font.italic: true; wrapMode: Text.WrapAtWordBoundaryOrAnywhere; textFormat: Text.RichText; width: parent.width-icon.width-5 }
                }
                Text {
                    id: text;
                    text: description;
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    textFormat: Text.RichText
                    width: parent.width-10
                }
                Text { visible: link !== ""; id: readmore; text: qsTr("Click to read more..."); font.italic: true; wrapMode: Text.WrapAtWordBoundaryOrAnywhere; textFormat: Text.RichText }
            }
            Components.QStyleItem { id: styleItem; cursor: "pointinghandcursor"; anchors.fill: column }
            MouseArea { anchors.fill: column; onClicked: Qt.openUrlExternally(link); hoverEnabled: true; id: mouseArea }

            StateGroup {
                id: activeState
                states: [ State { name: "active"; when: delegateItem.active; PropertyChanges { target: delegateItem; opacity: 1 } } ]
                transitions: [
                    Transition { from: ""; to: "active"; reversible: true; NumberAnimation { target: delegateItem; property: "opacity"; duration: 200 } }
                ]
            }

            states: [
                State { name: "clicked"; when: mouseArea.pressed;  PropertyChanges { target: text; color: "black" } },
                State { name: "hovered"; when: mouseArea.containsMouse;  PropertyChanges { target: text; color: "#074C1C" } }
            ]

        }
    }
}
