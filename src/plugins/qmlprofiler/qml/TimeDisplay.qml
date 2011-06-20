import QtQuick 1.0
import Monitor 1.0
import "MainView.js" as Plotter

TiledCanvas {
    id: timeDisplay

    canvasSize {
        width: timeDisplay.width
        height: timeDisplay.height
    }
    tileSize.width: width
    tileSize.height: height
    canvasWindow.width:  width
    canvasWindow.height: height


    Component.onCompleted: {
        requestPaint();
    }

    property variant startTime;
    property variant endTime;

    onStartTimeChanged: requestPaint();
    onEndTimeChanged: requestPaint();
    onWidthChanged: requestPaint();
    onHeightChanged: requestPaint();

    property variant timePerPixel;

    onDrawRegion: {
        drawBackgroundBars( ctxt, region );

        var totalTime = endTime - startTime;
        var spacing = width / totalTime;

        var initialBlockLength = 120;
        var timePerBlock = Math.pow(2, Math.floor( Math.log( totalTime / width * initialBlockLength ) / Math.LN2 ) );
        var pixelsPerBlock = timePerBlock * spacing;
        var pixelsPerSection = pixelsPerBlock / 5;
        var blockCount = width / pixelsPerBlock;

        var realStartTime = Math.floor(startTime/timePerBlock) * timePerBlock;
        var realStartPos = (startTime-realStartTime) * spacing;

        timePerPixel = timePerBlock/pixelsPerBlock;


        ctxt.fillStyle = "#000000";
        ctxt.font = "8px sans-serif";
        for (var ii = 0; ii < blockCount+1; ii++) {
            var x = Math.floor(ii*pixelsPerBlock - realStartPos);
            ctxt.strokeStyle = "#909090";
            ctxt.beginPath();
            ctxt.moveTo(x, 0);
            ctxt.lineTo(x, height);
            ctxt.stroke();

            ctxt.strokeStyle = "#C0C0C0";
            for (var jj=1; jj < 5; jj++) {
                var xx = Math.floor(ii*pixelsPerBlock + jj*pixelsPerSection - realStartPos);
                ctxt.beginPath();
                ctxt.moveTo(xx, labels.y);
                ctxt.lineTo(xx, height);
                ctxt.stroke();
            }

            ctxt.fillText(prettyPrintTime(ii*timePerBlock + realStartTime), x + 5, 10);
        }
    }

    function drawBackgroundBars( ctxt, region ) {
        var barHeight = labels.height / labels.rowCount;
        var originY = labels.y
        for (var i=0; i<labels.rowCount; i++) {
            ctxt.fillStyle = i%2 ? "#f3f3f3" : "white"
            ctxt.strokeStyle = i%2 ? "#f3f3f3" : "white"
            ctxt.fillRect(0, i * barHeight + originY, width, barHeight);
        }

        ctxt.fillStyle = "white";
        ctxt.fillRect(0, 0, width, originY);
    }

    function prettyPrintTime( t )
    {
        if (t <= 0) return "0";
        if (t<1000) return t+" ns";
        t = t/1000;
        if (t<1000) return t+" μs";
        t = Math.floor(t/100)/10;
        if (t<1000) return t+" ms";
        t = Math.floor(t/100)/10;
        if (t<60) return t+" s";
        t = Math.floor(t)/60;
        if (t<60) return t+" m";
        t = Math.floor(t)/60;
        return t+" h";
    }

    function detailedPrintTime( t )
    {
        if (t <= 0) return "0";
        if (t<1000) return t+" ns";
        t = Math.floor(t/1000);
        if (t<1000) return t+" μs";
        return (t/1000) + " ms";
    }

    // show exact time
    MouseArea {
        width: parent.width
        height: labels.y
        hoverEnabled: true
        onMousePositionChanged: {
            var realTime = startTime + mouseX * timePerPixel;
            displayText.text = detailedPrintTime(realTime);
            displayRect.x = mouseX
            displayRect.visible = true
        }
        onExited: displayRect.visible = false
    }

    Rectangle {
        id: displayRect
        color: "lightsteelblue"
        border.color: Qt.darker(color)
        border.width: 1
        radius: 2
        height: labels.y
        width: displayText.width + 10
        visible: false
        Text {
            id: displayText
            x: 5
            font.pointSize: 8
        }
    }
}
