import QtQuick
import %{ImportModuleName}
@if %{UseVirtualKeyboard}
import QtQuick.VirtualKeyboard
@endif

Window {
    width: mainScreen.width
    height: mainScreen.height

    visible: true
    title: "%{ProjectName}"

    %{UIClassName} {
        id: mainScreen

        anchors.centerIn: parent
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

