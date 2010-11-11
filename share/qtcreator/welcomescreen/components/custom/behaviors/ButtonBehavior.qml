import QtQuick 1.0

Item {
    id: behavior

    signal clicked
    property bool pressed: false    // Can't be alias of mouseArea.pressed because the latter is read-only
    property alias containsMouse: mouseArea.containsMouse
    property bool checkable: false
    property bool checked: false
    property bool triState: false
    signal mouseMoved

    onCheckableChanged: { if(!checkable) checked = false }
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onPositionChanged: behavior.mouseMoved()
        onPressed: behavior.pressed = true  // needed when hover is enabled
        onEntered: if(pressed && enabled) behavior.pressed = true
        onExited: behavior.pressed = false
        onCanceled: behavior.pressed = false    // mouse stolen e.g. by Flickable
        onReleased: {
            if(behavior.pressed && behavior.enabled) { // No click if release outside area
                behavior.pressed = false
                if(behavior.checkable)
                    behavior.checked = !behavior.checked;
                behavior.clicked()
            }
        }
    }
}
