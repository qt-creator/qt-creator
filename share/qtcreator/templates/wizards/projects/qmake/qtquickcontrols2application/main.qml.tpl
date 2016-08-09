import QtQuick %{QtQuickVersion}
import QtQuick.Controls %{QtQuickControls2Version}
import QtQuick.Layouts %{QtQuickLayoutsVersion}
@if %{UseVirtualKeyboard}
import QtQuick.VirtualKeyboard %{QtQuickVirtualKeyboardVersion}
@endif

ApplicationWindow {
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello World")
@if %{UseVirtualKeyboard}
    id: root
@endif

    SwipeView {
        id: swipeView
        anchors.fill: parent
        currentIndex: tabBar.currentIndex

        Page1 {
        }

        Page {
            Label {
                text: qsTr("Second page")
                anchors.centerIn: parent
            }
        }
    }

    footer: TabBar {
        id: tabBar
        currentIndex: swipeView.currentIndex
        TabButton {
            text: qsTr("First")
        }
        TabButton {
            text: qsTr("Second")
        }
    }
@if %{UseVirtualKeyboard}

    InputPanel {
        id: inputPanel
        z: 99
        x: 0
        y: root.height
        width: root.width

        states: State {
            name: "visible"
            when: inputPanel.active
            PropertyChanges {
                target: inputPanel
                y: root.height - inputPanel.height
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
