import QtQuick 1.1
import Monitor 1.0
import "MainView.js" as Plotter

BorderImage {
    id: rangeDetails

    property string duration    //###int?
    property string label
    property string type
    property string file
    property int line

    source: "popup.png"
    border {
        left: 10; top: 10
        right: 20; bottom: 20
    }

    width: col.width + 45
    height: childrenRect.height + 30
    z: 1
    visible: false

    //title
    Text {
        id: typeTitle
        text: rangeDetails.type
        font.bold: true
        y: 10
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: -5
    }

    //details
    Column {
        id: col
        anchors.top: typeTitle.bottom
        Detail {
            label: "Duration"
            content: rangeDetails.duration + "μs"
        }
        Detail {
            opacity: content.length !== 0 ? 1 : 0
            label: "Details"
            content: rangeDetails.label
        }
        Detail {
            opacity: content.length !== 0 ? 1 : 0
            label: "Location"
            content: {
                var file = rangeDetails.file
                var pos = file.lastIndexOf("/")
                if (pos != -1)
                    file = file.substr(pos+1)
                return (file.length !== 0) ? (file + ":" + rangeDetails.line) : "";
            }
            onLinkActivated: Qt.openUrlExternally(url)
        }
    }
}
