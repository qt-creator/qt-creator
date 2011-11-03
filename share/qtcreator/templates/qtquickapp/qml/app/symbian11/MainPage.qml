import QtQuick 1.1
import com.nokia.symbian 1.1

Page {
    id: mainPage
    Text {
        anchors.centerIn: parent
        text: qsTr("Hello world!")
        color: platformStyle.colorNormalLight
        font.pixelSize: 20
    }
}
