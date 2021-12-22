import QtQuick 2.15

GridView {
    width: 420
    height: 420

    cellWidth: 140
    cellHeight: 140

    children: [
        Rectangle {
            color: "#1d1d1d"
            anchors.fill: parent
            z: -1
        }
    ]

    model: %{ModelName} {
    }

    highlight: Rectangle {
        width: 120
        height: 120
        color: "#343434"
        radius: 4
        border.color: "#0d52a4"
        border.width: 8
    }

    delegate: %{DelegateName} {
    }
}
