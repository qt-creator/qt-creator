import QtQuick 2.10
import QtQuick.Templates 2.1 as T
import Home 1.0

T.Button {
    id: control
    width: 296
    height: 538

    font: Constants.font
    implicitWidth: Math.max(
                       background ? background.implicitWidth : 0,
                       contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(
                        background ? background.implicitHeight : 0,
                        contentItem.implicitHeight + topPadding + bottomPadding)
    leftPadding: 4
    rightPadding: 4

    background: normalBG
    contentItem: normalContent

    Item {
        id: normalBG
    }
    Item {
        id: normalContent
    }

    Item {
        id: focusedBG
    }
    Item {
        id: focusedContent
    }
    Item {
        id: pressedBG
    }
    Item {
        id: defaultBG
    }

    states: [
        State {
            name: "normal"
            when: !control.down && !control.focus

            PropertyChanges {
                target: focusedBG
                visible: false
            }
            PropertyChanges {
                target: focusedContent
                visible: false
            }
            PropertyChanges {
                target: pressedBG
                visible: false
            }
        },
        State {
            name: "press"
            when: control.down && control.focus
            PropertyChanges {
                target: control
                contentItem: focusedContent
            }

            PropertyChanges {
                target: normalBG
                visible: false
            }

            PropertyChanges {
                target: normalContent
                visible: false
            }

            PropertyChanges {
                target: pressedBG
                visible: true
            }

            PropertyChanges {
                target: focusedContent
                visible: true
            }

            PropertyChanges {
                target: control
                background: pressedBG
            }
        }
    ]
    QtObject {
        id: qds_stylesheet_merger_options
        property bool useStyleSheetPositions: true
    }
}
