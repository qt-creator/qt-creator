import QtQuick

ListModel {
    id: eventListModel

    ListElement {
        eventId: "enterPressed"
        eventDescription: "Emitted when pressing the enter button"
        shortcut: "Return"
        parameters: "Enter"
    }
}
