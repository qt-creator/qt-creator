import QtQuick 1.0
import "custom" as Components
import "plugin"

Components.Button {
    id:button

    height: 40; //styleitem.sizeFromContents(32, 32).height
    width: 40; //styleitem.sizeFromContents(32, 32).width

    QStyleItem {elementType: "toolbutton"; id:styleitem  }

    background: QStyleItem {
        anchors.fill: parent
        id: styleitem
        elementType: "toolbutton"
        on: pressed | checked
        sunken: pressed
        raised: containsMouse
        hover: containsMouse

        Text {
            text: button.text
            anchors.centerIn: parent
            visible: button.iconSource == ""
        }

        Image {
            source: button.iconSource
            anchors.centerIn: parent
            opacity: enabled ? 1 : 0.5
        }
    }
}
