import QtQuick 1.1
import Monitor 1.0
import "MainView.js" as Plotter

Text {
    id: elapsed
    color: "white"

    Timer {
        property date startDate
        property bool reset:  true
        running: connection.recording
        repeat: true
        onRunningChanged: if (running) reset = true
        interval:  100
        triggeredOnStart: true
        onTriggered: {
            if (reset) {
                startDate = new Date()
                reset = false
            }
            var time = (new Date() - startDate)/1000
            elapsed.text = time.toFixed(1) + "s"
        }
    }
}
