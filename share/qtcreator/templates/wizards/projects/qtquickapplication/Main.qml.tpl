import QtQuick
@if %{UseVirtualKeyboard}
import QtQuick.VirtualKeyboard
@endif

Window {
@if %{UseVirtualKeyboard}
    id: window
@endif
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")
@if %{UseVirtualKeyboard}

    InputPanel {
        id: inputPanel
        z: 99
        y: window.height
        width: window.width

        states: State {
            name: "visible"
            when: inputPanel.active
            PropertyChanges {
                inputPanel.y: window.height - inputPanel.height
            }
        }
        transitions: Transition {
            from: ""
            to: "visible"
            reversible: true
            NumberAnimation {
                properties: "y"
                easing.type: Easing.InOutQuad
            }
        }
    }
@endif
}
