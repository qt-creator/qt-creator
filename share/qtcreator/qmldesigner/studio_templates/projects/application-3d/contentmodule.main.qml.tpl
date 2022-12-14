import QtQuick %{QtQuickVersion}
@if !%{IsQt6Project}
import QtQuick.Window %{QtQuickVersion}
@endif
import %{ApplicationImport}
@if %{UseVirtualKeyboard}
import QtQuick.VirtualKeyboard %{QtQuickVersion}
@endif

Window {
    width: Constants.width
    height: Constants.height

    visible: true

    Screen01 {
    }

@if %{UseVirtualKeyboard}
    InputPanel {
        id: inputPanel
        property bool showKeyboard :  active
        y: showKeyboard ? parent.height - height : parent.height
        Behavior on y {
            NumberAnimation {
                duration: 200
                easing.type: Easing.InOutQuad
            }
        }
        anchors.leftMargin: Constants.width/10
        anchors.rightMargin: Constants.width/10
        anchors.left: parent.left
        anchors.right: parent.right
    }
@endif
}
