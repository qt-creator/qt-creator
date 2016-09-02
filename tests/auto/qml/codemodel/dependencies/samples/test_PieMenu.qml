import QtQuick 2.4
import QtQuick.Extras 1.4
import QtQuick.Controls 1.4

PieMenu {
    id: pieMenu

    MenuItem {
        text: "Action 1"
        onTriggered: print("Action 1")
    }
    MenuItem {
        text: "Action 2"
        onTriggered: print("Action 2")
    }
    MenuItem {
        text: "Action 3"
        onTriggered: print("Action 3")
    }
}
