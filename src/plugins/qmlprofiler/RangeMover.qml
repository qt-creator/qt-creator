import "MainView.js" as Plotter
import QtQuick 1.1
import Monitor 1.0

Item {
    id: rangeMover
    width: rect.width; height: 50

    property real prevXStep: -1
    property real possibleValue: (canvas.canvasWindow.x + x) * Plotter.xScale(canvas)
    onPossibleValueChanged:  {
        var set = (!zooming || (zooming && prevXStep != canvas.canvasWindow.x))
        prevXStep = canvas.canvasWindow.x;
        if (set)
            value = possibleValue
    }

    property real value
    //onValueChanged: console.log("*************** " + value)

    /*Image {
        id: leftRange
        source: "range.png"
        anchors.horizontalCenter: parent.left
        anchors.bottom: parent.bottom
    }*/

    Rectangle {
        id: rect

        color: "#cc80b2f6"
        width: 20
        //anchors.left: parent.left
        //anchors.right: rightRange.horizontalCenter

        height: parent.height
    }

    /*Image {
        id: rightRange
        source: "range.png"
        x: 13
        anchors.bottom: parent.bottom

        MouseArea {
            width: parent.width
            height: 15
            drag.target: rightRange
            drag.axis: "XAxis"
            drag.minimumX: -7    //###
            drag.maximumX: canvas.width - rangeMover.width  //###
        }
    }*/

    MouseArea {
        anchors.fill: parent
        drag.target: rangeMover
        drag.axis: "XAxis"
        drag.minimumX: 0
        drag.maximumX: canvas.width - rangeMover.width //###
    }
}
