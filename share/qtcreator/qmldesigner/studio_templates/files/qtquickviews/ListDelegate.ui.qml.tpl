/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/

import QtQuick 2.15

Item {
    id: delegate
    width: ListView.view.width
    height: 140

    Rectangle {
        id: rectangle
        color: "#bdbdbd"
        anchors.fill: parent
        anchors.margins: 12
        visible: true
        radius: 4
    }

    Text {
        id: label
        color: "#343434"
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter

        font.family: Constants.largeFont.family
        text: name
        anchors.margins: 24
    }
    MouseArea {
        anchors.fill: parent
        onClicked: delegate.ListView.view.currentIndex = index
    }
    states: [
        State {
            name: "Highlighted"

            when: delegate.ListView.isCurrentItem
            PropertyChanges {
                target: label
                color: "#efefef"
                anchors.topMargin: 52
            }

            PropertyChanges {
                target: rectangle
                visible: false
            }
        }
    ]
}
