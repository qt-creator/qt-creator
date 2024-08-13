// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T

T.Button {
    id: root

    property int warningCount: 0
    property int errorCount: 0

    property bool hasWarnings: warningCount > 0
    property bool hasErrors: errorCount > 0

    width: 136
    height: 29
    checkable: true
    state: "idleEmpty"

    Rectangle {
        id: bck
        color: "#202020"
        radius: 5
        border.color: "#444444"
        anchors.fill: parent

        Text {
            id: warningNumLabel
            color: "#ffffff"
            text: root.warningCount
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: warningsIcon.left
            anchors.rightMargin: 8
            font.pixelSize: 12
            horizontalAlignment: Text.AlignRight
        }

        Text {
            id: errorNumLabel
            color: "#ffffff"
            text: root.errorCount
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: errorIcon.right
            anchors.leftMargin: 8
            font.pixelSize: 12
            horizontalAlignment: Text.AlignLeft
        }

        Image {
            id: errorIcon
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 47
            source: "images/errorPassive.png"
            fillMode: Image.PreserveAspectFit
        }

        Image {
            id: warningsIcon
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 47
            source: "images/warningsPassive.png"
            fillMode: Image.PreserveAspectFit
        }
    }
    states: [
        State {
            name: "idleEmpty"
            when: !root.hovered && !root.checked && !root.pressed
                  && !root.hasErrors && !root.hasWarnings

            PropertyChanges {
                target: warningNumLabel
                text: "0"
            }

            PropertyChanges {
                target: errorNumLabel
                text: "0"
            }
        },
        State {
            name: "idleHover"
            when: root.hovered && !(root.checked || root.pressed)
                  && !root.hasErrors && !root.hasWarnings
            PropertyChanges {
                target: warningNumLabel
                text: "0"
            }

            PropertyChanges {
                target: errorNumLabel
                text: "0"
            }

            PropertyChanges {
                target: bck
                color: "#2d2d2d"
            }
        },

        State {
            name: "pressCheck"
            when: (root.checked || root.pressed) && !root.hasErrors && !root.hasWarnings
            PropertyChanges {
                target: warningNumLabel
                text: "0"
            }

            PropertyChanges {
                target: errorNumLabel
                text: "0"
            }

            PropertyChanges {
                target: bck
                color: "#2d2d2d"
                border.color: "#2eb0f9"
            }
        },

        State {
            name: "idleWarningsOnly"
            when: !root.hovered && !root.checked && !root.pressed
                  && !root.hasErrors && root.hasWarnings

            PropertyChanges {
                target: warningsIcon
                source: "images/warningsActive.png"
            }

            PropertyChanges {
                target: warningNumLabel
                color: "#ffbb0c"
            }

            PropertyChanges {
                target: errorNumLabel
                text: "0"
            }
        },
        State {
            name: "hoverWarningsOnly"
            when: root.hovered && !root.checked && !root.pressed
                  && !root.hasErrors && root.hasWarnings
            PropertyChanges {
                target: warningsIcon
                source: "images/warningsActive.png"
            }

            PropertyChanges {
                target: warningNumLabel
                color: "#ffbb0c"
            }

            PropertyChanges {
                target: errorNumLabel
                text: "0"
            }

            PropertyChanges {
                target: bck
                color: "#2d2d2d"
            }
        },

        State {
            name: "pressCheckWarningsOnly"
            when: (root.checked || root.pressed) && !root.hasErrors
                  && root.hasWarnings
            PropertyChanges {
                target: warningsIcon
                source: "images/warningsActive.png"
            }

            PropertyChanges {
                target: warningNumLabel
                color: "#ffbb0c"
            }

            PropertyChanges {
                target: errorNumLabel
                text: "0"
            }

            PropertyChanges {
                target: bck
                color: "#2d2d2d"
                border.color: "#57b9fc"
            }
        },

        State {
            name: "idleErrorsOnly"
            when: !root.hovered && !root.checked && !root.pressed
                  && root.hasErrors && !root.hasWarnings

            PropertyChanges {
                target: warningNumLabel
                text: "0"
            }

            PropertyChanges {
                target: errorIcon
                source: "images/errorActive.png"
            }

            PropertyChanges {
                target: errorNumLabel
                color: "#ff4848"
            }
        },
        State {
            name: "hoverErrorsOnly"
            when: root.hovered && !root.checked && !root.pressed
                  && root.hasErrors && !root.hasWarnings
            PropertyChanges {
                target: warningNumLabel
                text: "0"
            }

            PropertyChanges {
                target: errorIcon
                source: "images/errorActive.png"
            }

            PropertyChanges {
                target: errorNumLabel
                color: "#ff4848"
            }

            PropertyChanges {
                target: bck
                color: "#2d2d2d"
            }
        },

        State {
            name: "pressCheckErrorsOnly"
            when: (root.checked || root.pressed) && root.hasErrors
                  && !root.hasWarnings
            PropertyChanges {
                target: warningNumLabel
                text: "0"
            }

            PropertyChanges {
                target: errorIcon
                source: "images/errorActive.png"
            }

            PropertyChanges {
                target: errorNumLabel
                color: "#ff4848"
            }

            PropertyChanges {
                target: bck
                color: "#2d2d2d"
                border.color: "#57b9fc"
            }
        },

        State {
            name: "idlesErrorsAndWarnings"
            when: !root.hovered && !root.checked && !root.pressed
                  && root.hasErrors && root.hasWarnings

            PropertyChanges {
                target: warningNumLabel
                color: "#ffbb0c"
            }

            PropertyChanges {
                target: warningsIcon
                source: "images/warningsActive.png"
            }

            PropertyChanges {
                target: errorNumLabel
                color: "#ff4848"
            }

            PropertyChanges {
                target: errorIcon
                source: "images/errorActive.png"
            }
        },
        State {
            name: "hoverErrorsAndWarnings"
            when: root.hovered && !root.checked && !root.pressed
                  && root.hasErrors && root.hasWarnings
            PropertyChanges {
                target: warningNumLabel
                color: "#ffbb0c"
            }

            PropertyChanges {
                target: warningsIcon
                source: "images/warningsActive.png"
            }

            PropertyChanges {
                target: errorNumLabel
                color: "#ff4848"
            }

            PropertyChanges {
                target: errorIcon
                source: "images/errorActive.png"
            }

            PropertyChanges {
                target: bck
                color: "#2d2d2d"
            }
        },
        State {
            name: "pressCheckWarningsErrors"
            when: (root.checked || root.pressed) && root.hasErrors
                  && root.hasWarnings
            PropertyChanges {
                target: warningNumLabel
                color: "#ffbb0c"
            }

            PropertyChanges {
                target: warningsIcon
                source: "images/warningsActive.png"
            }

            PropertyChanges {
                target: errorNumLabel
                color: "#ff4848"
            }

            PropertyChanges {
                target: errorIcon
                source: "images/errorActive.png"
            }

            PropertyChanges {
                target: bck
                color: "#2d2d2d"
                border.color: "#2eb0f9"
            }
        }
    ]
}
