import QtQuick 1.0
import "custom" as Components

Components.Switch {
    id:widget
    minimumWidth:100
    minimumHeight:30

    groove:QStyleItem {
        elementType:"edit"
        sunken: true
    }

    handle: QStyleItem {
        elementType:"button"
        width:widget.width/2
        height:widget.height-4
        hover:containsMouse
    }
}

