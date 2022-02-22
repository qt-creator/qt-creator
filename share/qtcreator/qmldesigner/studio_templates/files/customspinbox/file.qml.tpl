/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/

import QtQuick 2.15
import QtQuick.Controls 2.15

SpinBox {
    id: control
    value: 50
    editable: true

    contentItem: textInput
    TextInput {
        id: textInput
        z: 2
        text: control.value

        font: control.font
        color: "#047eff"
        selectionColor: "#047eff"
        selectedTextColor: "#ffffff"
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter

        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: Qt.ImhFormattedNumbersOnly
    }

    up.indicator: upIndicator

    Rectangle {
        id: upIndicator
        x: control.mirrored ? 0 : parent.width - width
        height: parent.height
        color: "#00000000"
        implicitWidth: 40
        implicitHeight: 40
        border.color: "#047eff"

        Text {
            id: text1
            text: "+"
            font.pixelSize: control.font.pixelSize * 2
            color: "#047eff"
            anchors.fill: parent
            fontSizeMode: Text.Fit
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    down.indicator: downIndicator
    Rectangle {
        id: downIndicator
        x: control.mirrored ? parent.width - width : 0
        height: parent.height
        color: "#00000000"
        implicitWidth: 40
        implicitHeight: 40
        border.color: "#047eff"

        Text {
            id: text2
            text: "-"
            font.pixelSize: control.font.pixelSize * 2
            color: "#047eff"
            anchors.fill: parent
            fontSizeMode: Text.Fit
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    background: backgroundRect
    Rectangle {
        id: backgroundRect
        color: "#00000000"
        implicitWidth: 140
        border.color: "#047eff"
    }
    states: [
        State {
            name: "normal"
            when: !control.down.pressed && !control.up.pressed
                  && control.enabled

            PropertyChanges {
                target: upIndicator
                border.color: "#047eff"
            }

            PropertyChanges {
                target: downIndicator
                border.color: "#047eff"
            }

            PropertyChanges {
                target: backgroundRect
                border.color: "#047eff"
            }

            PropertyChanges {
                target: textInput
                color: "#047eff"
                selectionColor: "#ffffff"
            }

            PropertyChanges {
                target: text1
                color: "#047eff"
            }

            PropertyChanges {
                target: text2
                color: "#047eff"
            }
        },
        State {
            name: "upPressed"
            when: control.up.pressed

            PropertyChanges {
                target: text1
                color: "#ffffff"
            }

            PropertyChanges {
                target: upIndicator
                color: "#047eff"
                border.color: "#047eff"
            }

            PropertyChanges {
                target: textInput
                color: "#047eff"
                selectionColor: "#ffffff"
            }

            PropertyChanges {
                target: downIndicator
                border.color: "#047eff"
            }

            PropertyChanges {
                target: text2
                color: "#047eff"
            }

            PropertyChanges {
                target: backgroundRect
                border.color: "#047eff"
            }
        },
        State {
            name: "downPressed"
            when: control.down.pressed

            PropertyChanges {
                target: text2
                color: "#ffffff"
            }

            PropertyChanges {
                target: downIndicator
                color: "#047eff"
                border.color: "#047eff"
            }

            PropertyChanges {
                target: textInput
                color: "#047eff"
                selectionColor: "#ffffff"
            }

            PropertyChanges {
                target: upIndicator
                border.color: "#047eff"
            }

            PropertyChanges {
                target: text1
                color: "#047eff"
            }

            PropertyChanges {
                target: backgroundRect
                border.color: "#047eff"
            }
        },
        State {
            name: "disabled"
            when: !control.enabled

            PropertyChanges {
                target: downIndicator
                border.color: "#b3047eff"
            }

            PropertyChanges {
                target: upIndicator
                border.color: "#b3047eff"
            }

            PropertyChanges {
                target: textInput
                color: "#b3047eff"
            }

            PropertyChanges {
                target: text1
                color: "#b3047eff"
            }

            PropertyChanges {
                target: text2
                color: "#b3047eff"
            }

            PropertyChanges {
                target: backgroundRect
                border.color: "#b3047eff"
            }
        }
    ]
}
