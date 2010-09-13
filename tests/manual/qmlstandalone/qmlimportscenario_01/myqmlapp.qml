import Qt 4.7
import "qmlproject01"
import "qmlproject02"

Rectangle {
    width: 200
    height: 200
    color: "#ddddff"

    Text {
        id: title
        text: "main"
    }

    Column {
        anchors.fill: parent
        anchors.margins: title.height + 2

        QmlProject01 {
            anchors.top: parent.top
            height: parent.height / 2
            width: parent.width
        }

        QmlProject02 {
            anchors.bottom: parent.bottom
            height: parent.height / 2
            width: parent.width
        }
    }
}
