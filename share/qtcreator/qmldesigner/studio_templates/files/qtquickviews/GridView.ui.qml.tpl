/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/

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
