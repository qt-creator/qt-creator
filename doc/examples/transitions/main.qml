import QtQuick 2.7

import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

ApplicationWindow {
    visible: true
    width: 640
    height: 480
    title: qsTr("Transitions")

    Page1Form {
        anchors.fill: parent
        id: page

        mouseArea {
        onClicked: stateGroup.state = ' '
        }
        mouseArea1 {
        onClicked: stateGroup.state = 'State1'
        }
        mouseArea2 {
        onClicked: stateGroup.state = 'State2'
        }
    }

    StateGroup {
        id: stateGroup
        states: [
            State {
                name: "State1"

                PropertyChanges {
                    target: page.icon
                    x: page.middleRightRect.x
                    y: page.middleRightRect.y
                }
            },
            State {
                name: "State2"

                PropertyChanges {
                    target: page.icon
                    x: page.bottomLeftRect.x
                    y: page.bottomLeftRect.y
                }
            }
        ]
        transitions: [
            Transition {
                from: "*"; to: "State1"
                NumberAnimation {
                    easing.type: Easing.OutBounce
                    properties: "x,y";
                    duration: 1000
                }
            },
            Transition {
                from: "*"; to: "State2"
                NumberAnimation {
                        properties: "x,y";
                        easing.type: Easing.InOutQuad;
                        duration: 2000
                }
            },
            Transition {
                     NumberAnimation {
                         properties: "x,y";
                         duration: 200
                     }
            }
        ]
    }
}
