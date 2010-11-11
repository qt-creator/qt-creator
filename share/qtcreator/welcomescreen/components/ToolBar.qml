import QtQuick 1.0
import "custom" as Components
import "plugin"

QStyleItem{
    id: toolbar
    width: 200
    height: sizeFromContents(32, 32).height
    elementType: "toolbar"
}

