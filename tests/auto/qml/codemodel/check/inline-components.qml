// LabeledImageBox.qml
import QtQuick 2.15
import "./InlineComponent.qml"

Rectangle {
    property alias caption: image.caption
    property alias source: image.source
    border.width: 2
    border.color: "black"
    InlineComponent.LabeledImage {
        id: image
    }
}
