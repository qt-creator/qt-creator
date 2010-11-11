import QtQuick 1.0
import "custom" as Components
import "plugin"


QStyleBackground {

    width: 100
    height: 100

    default property alias children: content.children

    style: QStyleItem {
        id: styleitem
        elementType: "frame"
    }

    Item {
        id: content
        anchors.fill: parent
        anchors.margins: frameWidth
        property int frameWidth: styleitem.pixelMetric("defaultframewidth");
    }
}

