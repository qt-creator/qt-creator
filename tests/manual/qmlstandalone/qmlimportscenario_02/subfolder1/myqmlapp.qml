import Qt 4.7
import no.trolltech.QmlModule01 1.0
import com.nokia.QmlModule02 1.0

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

        QmlComponent01 {
            height: parent.height / 2
            width: parent.width
        }

        QmlComponent02 {
            height: parent.height / 2
            width: parent.width
        }
    }
}
