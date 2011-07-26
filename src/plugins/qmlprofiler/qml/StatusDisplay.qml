import QtQuick 1.0
import "MainView.js" as Plotter

Rectangle {
    id: statusDisplay

    property real percentage : 0
    property int eventCount:  root.eventCount
    onEventCountChanged:  {
        if (state=="loading" && eventCount > 0 && root.elapsedTime > 0) {
            percentage = Math.min(1.0,
                (Plotter.ranges[Plotter.ranges.length-1].start - Plotter.ranges[0].start) / root.elapsedTime * 1e-9 );
        }
    }

    width: Math.max(200, statusText.width+20);
    height: displayColumn.height + 20

    visible: false;

    color: "#CCD0CC"
    border.width: 1
    border.color: "#AAAEAA";
    radius: 4

    Column {
        id: displayColumn
        y: 10
        spacing: 5
        width: parent.width
        Text {
            id: statusText
            horizontalAlignment: "AlignHCenter"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Rectangle {
            id: progressBar

            visible: false

            width: statusDisplay.width - 20
            height: 20
            x: 10
            color: "transparent"
            border.width: 1
            border.color: "#AAAEAA"
            Rectangle {
                x: 1
                y: 1
                width: (parent.width-1) * statusDisplay.percentage
                color: Qt.rgba(0.37 + 0.2*(1 - statusDisplay.percentage), 0.58, 0.37, 1);
                height: parent.height-1
            }
        }
    }

    states: [
        // no data available
        State {
            when: (root.eventCount == 0) && !elapsedTimer.running;
            PropertyChanges {
                target: statusDisplay
                visible: true
            }

            PropertyChanges {
                target: statusText
                text: qsTr("No QML events recorded");
            }
        },
        // running app
        State {
            when: elapsedTimer.running;
            PropertyChanges {
                target: statusDisplay
                visible: true
            }

            PropertyChanges {
                target: statusText
                text: qsTr("Profiling application");
            }
        },
        // loading data
        State {
            name: "loading"
            when: (!root.dataAvailable) && (root.eventCount > 0);
            PropertyChanges {
                target: statusDisplay
                visible: true
            }

            PropertyChanges {
                target: statusText
                text: qsTr("Loading data");
            }

            PropertyChanges {
                target: progressBar
                visible: true
            }
        }
    ]

}
