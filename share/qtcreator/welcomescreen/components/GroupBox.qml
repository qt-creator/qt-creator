import QtQuick 1.0
import "custom" as Components
import "plugin"

Components.GroupBox {
    id: groupbox
    width: Math.max(200, contentWidth + sizeHint.width)
    height: contentHeight + sizeHint.height + 4
    property variant sizeHint: backgroundItem.sizeFromContents(0, 24)
    property bool flat: false
    background : QStyleItem {
        id: styleitem
        elementType: "groupbox"
        anchors.fill: parent
        text: groupbox.title
        hover: checkbox.containsMouse
        on: checkbox.checked
        focus: checkbox.activeFocus
        activeControl: checkable ? "checkbox" : ""
        sunken: !flat
    }
}
