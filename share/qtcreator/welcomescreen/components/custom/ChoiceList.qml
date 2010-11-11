import QtQuick 1.0
import "./private" as Private

Item {
    id: choiceList

    property alias model: popup.model
    property alias currentIndex: popup.currentIndex
    property alias currentText: popup.currentText
    property alias popupOpen: popup.popupOpen
    property alias containsMouse: popup.containsMouse
    property alias pressed: popup.buttonPressed

    property Component background: null
    property Item backgroundItem: backgroundLoader.item
    property Component listItem: null
    property Component popupFrame: null

    property int leftMargin: 0
    property int topMargin: 0
    property int rightMargin: 0
    property int bottomMargin: 0

    property string popupBehavior
    width: 0
    height: 0

    property bool activeFocusOnPress: true

    Loader {
        id: backgroundLoader
        property alias styledItem: choiceList
        sourceComponent: background
        anchors.fill: parent
        property string currentItemText: model.get(currentIndex).text
    }

    Private.ChoiceListPopup {
        // NB: This ChoiceListPopup is also the mouse area
        // for the component (to enable drag'n'release)
        id: popup
        listItem: choiceList.listItem
        popupFrame: choiceList.popupFrame
    }

    Keys.onSpacePressed: { choiceList.popupOpen = !choiceList.popupOpen }
    Keys.onUpPressed: { if (currentIndex < model.count - 1) currentIndex++ }
    Keys.onDownPressed: {if (currentIndex > 0) currentIndex-- }
}
