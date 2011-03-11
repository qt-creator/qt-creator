import QtQuick 1.1
import Monitor 1.0
import "MainView.js" as Plotter

Item {
    id: detail
    property string label
    property string content
    property int maxLines: 4
    signal linkActivated(string url)

    height: childrenRect.height
    width: childrenRect.width
    Item {
        id: guideline
        x: 70
        width: 5
    }
    Text {
        id: lbl
        text: label + ":"
        font.pixelSize: 12
        font.bold: true
        anchors.right: guideline.left
    }
    Text {
        text: content
        font.pixelSize: 12
        anchors.baseline: lbl.baseline
        anchors.left: guideline.right
        maximumLineCount: maxLines
        onLinkActivated: detail.linkActivated(link)
    }
}
