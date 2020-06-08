import QtQuick 2.10
import QtQuick.Templates 2.1 as T
import TemplateMerging 1.0

T.Switch {
    id: control

    implicitWidth: background.implicitWidth
    implicitHeight: background.implicitHeight

    text: "test"

    background: Item {
        implicitWidth: switchBackground.width
        implicitHeight: switchBackground.height
        Rectangle {
            id: switchBackground
            color: "#ef1d1d"
            border.color: "#808080"
            width: 12 * 6.0
            height: 12 * 3.8
            anchors.fill: parent // has to be preserved
            Text {
                id: switchBackgroundText
                anchors.right: parent.right // does have to be preserved -- how to handle this? - anchors preference from style if not "root"?

                anchors.verticalCenter: parent.verticalCenter // does have to be preserved -- how to handle this? - anchors preference from style if not "root"?
                text: control.text // has to be preserved
                anchors.rightMargin: 12 * 5
            }
        }
    }

    leftPadding: 4

    contentItem: Item  { //designer want to edit the label as part of background
    }


    indicator: Rectangle {
        id: switchIndicator
        width: 58
        height: 31
        x: control.leftPadding // has to be preserved
        color: "#e9e9e9"
        anchors.verticalCenter: parent.verticalCenter // has to be preserved
        radius: 16
        border.color: "#dddddd"

        Rectangle {
            id: switchHandle //id is required for states

            width: 31
            height: 31
            radius: 16
            color: "#e9e9e9"
            border.color: "#808080"
        }
    }
    states: [
        State {
            name: "off"
            when: !control.checked && !control.down
        },
        State {
            name: "on"
            when: control.checked && !control.down

            PropertyChanges {
                target: switchIndicator
                color: "#1713de"
                border.color: "#1713de"
            }

            PropertyChanges {
                target: switchHandle
                x: parent.width - width
            }
        },
        State {
            name: "off_down"
            when: !control.checked && control.down

            PropertyChanges {
                target: switchIndicator
                color: "#e9e9e9"
            }

            PropertyChanges {
                target: switchHandle
                color: "#d2d2d2"
                border.color: "#d2d2d2"
            }
        },
        State {
            name: "on_down"
            when: control.checked && control.down

            PropertyChanges {
                target: switchHandle
                x: parent.width - width
                color: "#e9e9e9"
            }

            PropertyChanges {
                target: switchIndicator
                color: "#030381"
                border.color: "#030381"
            }
        }
    ]
}


