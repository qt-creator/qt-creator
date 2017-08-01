import QtQuick %{QtQuickVersion}
import QtQuick.Window %{QtQuickWindowVersion}
@if %{UseVirtualKeyboard}
import %{QtQuickVirtualKeyboardImport}
@endif

Window {
@if %{UseVirtualKeyboard}
    id: window
@endif
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello World")
@if %{UseVirtualKeyboard}

    InputPanel {
        id: inputPanel
        z: 99
        x: 0
        y: window.height
        width: window.width

        states: State {
            name: "visible"
            when: inputPanel.active
            PropertyChanges {
                target: inputPanel
                y: window.height - inputPanel.height
            }
        }
        transitions: Transition {
            from: ""
            to: "visible"
            reversible: true
            ParallelAnimation {
                NumberAnimation {
                    properties: "y"
                    duration: 250
                    easing.type: Easing.InOutQuad
                }
            }
        }
    }
@endif
}
