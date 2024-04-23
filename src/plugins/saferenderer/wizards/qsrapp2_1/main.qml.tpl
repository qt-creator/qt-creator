import Qt.SafeRenderer
import QtQuick.Window

Window {
    id: window
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello QSR")

    SafeText {
        id: safeText
        objectName: "safetextitem"
        x: 206
        y: 208
        width: 340
        height: 40
        color: "#8ae234"
        fillColor: "black"
        text: "Hello Qt Safe Renderer!"
        font.family: "Lato"
        horizontalAlignment: Text.AlignLeft
        font.pixelSize: 32
    }
}
