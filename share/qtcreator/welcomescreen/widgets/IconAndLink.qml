import QtQuick 1.1
import widgets 1.0

Row {
    id: root

    property string iconName
    property string linkText
    signal clicked

    spacing: 4

    Image {
        y: 10
        source: "images/icons/" + iconName +".png"
    }

    LinkedText {
        x: 19
        y: 5
        height: 38
        text: linkText
        verticalAlignment: Text.AlignBottom
        onClicked: root.clicked();
    }
}
