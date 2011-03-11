import QtQuick 1.1
import Monitor 1.0
import "MainView.js" as Plotter

Rectangle {
    id: root

    property variant colors:  Plotter.colors  //the colors used for the timeline data
    property bool xRay: false                 //useful for seeing "nested" ranges (but redraw is buggy -- QGV problem?)
    property Item currentItem                 //currently selected item in the view

    property bool zooming:false

    // move the cursor in the editor
    signal updateCursorPosition
    property string fileName: ""
    property int lineNumber: -1
    function gotoSourceLocation(file,line) {
        root.fileName = file;
        root.lineNumber = line;
        root.updateCursorPosition();
    }

    //handle debug data coming from C++
    Connections {
        target: connection
        onEvent: {
            if (Plotter.valuesdone) {
                Plotter.reset();
                view.clearData();
                rangeMover.x = 2
                rangeMover.opacity = 0
            }

            if (!Plotter.valuesdone && event === 0) //### only handle paint event
                Plotter.values.push(time);
        }

        onRange: {
            if (Plotter.valuesdone) {
                Plotter.reset();
                view.clearData();
                rangeMover.x = 2
                rangeMover.opacity = 0
            }

            if (!Plotter.valuesdone)
                Plotter.ranges.push( { type: type, start: startTime, duration: length, label: data, fileName: fileName, line: line } );
        }

        onComplete: {
            Plotter.valuesdone = true;
            Plotter.calcFps();
            view.setRanges(Plotter.ranges);
            view.updateTimeline();
            canvas.requestPaint();
            rangeMover.x = 1    //### hack to get view to display things immediately
            rangeMover.opacity = 1
        }

    }

    //timeline background
    Item {
        anchors.fill: flick
        Column {
            anchors.fill: parent

            Repeater {
                model: 5 //### values.length?
                delegate: Rectangle {
                    width:  parent.width
                    height: 50 //###
                    color: index % 2 ? "#fafafa" : "white"
                }
            }
        }
    }

    //our main interaction view
    Flickable {
        id: flick
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.left: labels.right
        anchors.bottom: canvas.top
        contentWidth: view.totalWidth
        contentHeight: view.height

        TimelineView {
            id: view

            width: flick.width; height: flick.height

            startX: flick.contentX
            onStartXChanged: {
                var newX = startTime / Plotter.xScale(canvas) - canvas.canvasWindow.x;
                if (Math.abs(rangeMover.x - newX) > .01)
                    rangeMover.x = newX
                if (Math.abs(startX - flick.contentX) > .01)
                    flick.contentX = startX
            }
            startTime: rangeMover.value

            property real prevXStep:  -1
            property real possibleEndTime: startTime + (rangeMover.width*Plotter.xScale(canvas))
            onPossibleEndTimeChanged:  {
                var set = ((zooming && prevXStep != canvas.canvasWindow.x) || !zooming);
                prevXStep = canvas.canvasWindow.x;
                if (set)
                    endTime = possibleEndTime
            }
            onEndTimeChanged: updateTimeline()

            delegate: Rectangle {
                id: obj

                property color baseColor: colors[type]
                property color myColor: baseColor

                function conditionalHide() {
                    if (!mouseArea.containsMouse)
                        mouseArea.exited()
                }

                height: 50
                gradient: Gradient {
                    GradientStop { position: 0.0; color: myColor }
                    GradientStop { position: 0.5; color: Qt.darker(myColor, 1.1) }
                    GradientStop { position: 1.0; color: myColor }
                }
                smooth: true
                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: {
                        currentItem = obj
                        myColor = Qt.darker(baseColor, 1.2)
                        rangeDetails.duration = duration
                        rangeDetails.label = label
                        rangeDetails.file = fileName
                        rangeDetails.line = line
                        rangeDetails.type = Plotter.names[type]

                        var pos = mapToItem(rangeDetails.parent, mouseX, y+height)
                        var preferredX = Math.max(10, pos.x - rangeDetails.width/2)
                        if (preferredX + rangeDetails.width > rangeDetails.parent.width)
                            preferredX = rangeDetails.parent.width - rangeDetails.width
                        rangeDetails.x = preferredX

                        rangeDetails.y = pos.y + 10
                        rangeDetails.visible = true
                    }
                    onExited: {
                        myColor = baseColor
                        rangeDetails.visible = false
                        rangeDetails.duration = ""
                        rangeDetails.label = ""
                        rangeDetails.type = ""
                        rangeDetails.file = ""
                        rangeDetails.line = -1
                    }
                    onClicked: root.gotoSourceLocation(rangeDetails.file, rangeDetails.line);
                }
            }
        }
    }

    //popup showing the details for the hovered range
    RangeDetails {
        id: rangeDetails
    }

    Rectangle {
        id: labels
        width: 150
        color: "#dcdcdc"
        anchors.top: root.top
        anchors.bottom: canvas.top

        Column {
            //### change to use Repeater + Plotter.names?
            Label { text: "Painting" }
            Label { text: "Compiling" }
            Label { text: "Creating" }
            Label { text: "Binding" }
            Label { text: "Signal Handler" }
        }

        //right border divider
        Rectangle {
            width: 1
            height: parent.height
            anchors.right: parent.right
            color: "#cccccc"
        }
    }

    //bottom border divider
    Rectangle {
        height: 1
        width: parent.width
        anchors.bottom: canvas.top
        color: "#cccccc"
    }

    //"overview" graph at the bottom
    TiledCanvas {
        id: canvas

        anchors.bottom: parent.bottom
        width: parent.width; height: 50

        property int canvasWidth: width

        canvasSize {
            width: canvasWidth
            height: canvas.height
        }

        tileSize.width: width
        tileSize.height: height

        canvasWindow.width: width
        canvasWindow.height: 50

        onDrawRegion: {
             if (Plotter.valuesdone)
                 Plotter.plot(canvas, ctxt, region);
             else
                 Plotter.drawGraph(canvas, ctxt, region)    //just draw the background
        }
    }

    //moves the range mover to the position of a click
    MouseArea {
        anchors.fill: canvas
        //### ideally we could press to position then immediately drag
        onPressed: rangeMover.x = mouse.x - rangeMover.width/2
    }

    RangeMover {
        id: rangeMover
        opacity: 0
        anchors.top: canvas.top
    }

    Rectangle {
        width: 50
        height: 30
        anchors.right: root.right
        anchors.top: root.top
        radius: 4
        color: "#606085"
        Elapsed {
            anchors.centerIn: parent
            horizontalAlignment: Text.AlignHCenter
        }
    }

}
