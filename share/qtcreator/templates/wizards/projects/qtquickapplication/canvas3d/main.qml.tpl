import QtQuick 2.4
import QtCanvas3D 1.1
import QtQuick.Window 2.2
@if %{UseVirtualKeyboard}
import %{QtQuickVirtualKeyboardImport}
@endif

import "glcode.js" as GLCode

Window {
@if %{UseVirtualKeyboard}
    id: window
@endif
    title: qsTr("%{ProjectName}")
    width: 640
    height: 360
    visible: true

    Canvas3D {
        id: canvas3d
        anchors.fill: parent
        focus: true

        onInitializeGL: {
            GLCode.initializeGL(canvas3d);
        }

        onPaintGL: {
            GLCode.paintGL(canvas3d);
        }

        onResizeGL: {
            GLCode.resizeGL(canvas3d);
        }
    }
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
