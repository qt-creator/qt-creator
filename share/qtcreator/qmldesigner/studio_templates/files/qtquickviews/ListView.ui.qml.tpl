import QtQuick 2.15

ListView {
    id: view
    width: 420
    height: 420

    highlightMoveDuration: 0

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
        width: view.width
        height: 120
        color: "#343434"
        radius: 4
        border.color: "#0d52a4"
        border.width: 8
    }

    delegate: %{DelegateName} {
    }
}
