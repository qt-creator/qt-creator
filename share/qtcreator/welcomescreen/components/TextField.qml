import QtQuick 1.0
import "custom" as Components
import "plugin"

Components.TextField {
    id: textfield
    minimumWidth: 200

    placeholderText: ""
    topMargin: 2
    bottomMargin: 2
    leftMargin: 6
    rightMargin: 6

    height:  backgroundItem.sizeFromContents(200, 25).height
    width: 200
    clip: false

    background: QStyleItem {
        anchors.fill: parent
        elementType: "edit"
        sunken: true
        focus: textfield.activeFocus
        hover: containsMouse
    }

    Item{
        id: focusFrame
        anchors.fill: textfield
        parent: textfield
        visible: framestyle.styleHint("focuswidget")
        QStyleItem {
            id: framestyle
            anchors.margins: -2
            anchors.rightMargin:-4
            anchors.bottomMargin:-4
            anchors.fill: parent
            visible: textfield.activeFocus
            elementType: "focusframe"
        }
    }
}
