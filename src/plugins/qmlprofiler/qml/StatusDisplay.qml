import QtQuick 1.0

Item {
    id: statusDisplay

    property real percentage : root.progress

    width: Math.max(200, statusText.width+20)
    height: displayColumn.height + 20

    visible: false;

    // shadow
    BorderImage {
        property int px: 4
        source: "dialog_shadow.png"

        border {
            left: px; top: px
            right: px; bottom: px
        }
        width: parent.width + 2*px - 1
        height: parent.height
        x: -px + 1
        y: px + 1
    }

    // background
    Rectangle {
        color: "#E0E0E0"
        border.width: 1
        border.color: "#666666"
        radius: 4
        anchors.fill: parent
    }

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
                color: Qt.rgba(0.37 + 0.2*(1 - statusDisplay.percentage), 0.58, 0.37, 1)
                height: parent.height-1
            }
        }
    }

    states: [
        // no data available
        State {
            when: (root.eventCount == 0) && (elapsedTime > 0) && !root.recordingEnabled
            PropertyChanges {
                target: statusDisplay
                visible: true
            }

            PropertyChanges {
                target: statusText
                text: qsTr("No QML events recorded")
            }
            onCompleted: {
                root.clearDisplay();
            }
        },
        // running app
        State {
            when: root.recordingEnabled
            PropertyChanges {
                target: statusDisplay
                visible: true
            }

            PropertyChanges {
                target: statusText
                text: qsTr("Profiling application")
            }
        },
        // loading data
        State {
            name: "loading"
            when: !root.dataAvailable && (root.eventCount > 0) && !root.appKilled
            PropertyChanges {
                target: statusDisplay
                visible: true
            }

            PropertyChanges {
                target: statusText
                text: qsTr("Loading data")
            }

            PropertyChanges {
                target: progressBar
                visible: true
            }
        },
        // application died
        State {
            name: "deadApp"
            when: !root.dataAvailable && (root.eventCount > 0) && root.appKilled
            PropertyChanges {
                target: statusDisplay
                visible: true
            }
            PropertyChanges {
                target: statusText
                text: qsTr("Application stopped before loading all data")
            }
            PropertyChanges {
                target: progressBar
                visible: true
            }
        }
    ]

}
