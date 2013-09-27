import QtQuick 2.1
import QtQuick.Controls 1.1 as Controls
import QtQuick.Controls.Styles 1.1

Controls.Button {
    property color borderColor: "#222"
    property color highlightColor: "orange"
    property color textColor: "#eee"
    style: ButtonStyle {
        label: Text {
            color: CreatorStyle.textColor
            anchors.fill: parent
            renderType: Text.NativeRendering
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            text: control.text
            opacity: enabled ? 1 : 0.7
        }
        background: Rectangle {
            implicitWidth: 100
            implicitHeight: 23
            border.color: CreatorStyle.borderColor
            radius: 3
            gradient: control.pressed ? pressedGradient : gradient
            Gradient{
                id: pressedGradient
                GradientStop{color: "#333" ; position: 0}
            }
            Gradient {
                id: gradient
                GradientStop {color: "#606060" ; position: 0}
                GradientStop {color: "#404040" ; position: 0.07}
                GradientStop {color: "#303030" ; position: 1}
            }
            Rectangle {
                border.color: CreatorStyle.highlightColor
                anchors.fill: parent
                anchors.margins: -1
                color: "transparent"
                radius: 4
                opacity: 0.3
                visible: control.activeFocus
            }
        }
    }
}
