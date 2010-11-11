import QtQuick 1.0
import "custom" as Components
import "plugin"

Components.ChoiceList {

    id: choicelist

    property int buttonHeight: backgroundItem.sizeFromContents(100, 18).height
    property int buttonWidth: backgroundItem.sizeFromContents(100, 18).width

    property string hint

    height: buttonHeight
    width: buttonWidth
    topMargin: 4
    bottomMargin: 4

    background: QStyleItem {
        anchors.fill: parent
        elementType: "combobox"
        sunken: pressed
        raised: !pressed
        hover: containsMouse
        enabled: choicelist.enabled
        text: currentItemText
        focus: choicelist.focus
        hint: choicelist.hint
    }

    listItem: Item {
        id:item

        height: 22
        anchors.left: parent.left
        width: choicelist.width
        QStyleItem {
            anchors.fill: parent
            elementType: "comboboxitem"
            text: itemText
            selected: highlighted

        }
    }
    popupFrame: QStyleItem {
        property string popupLocation: backgroundItem.styleHint("comboboxpopup") ? "center" : "below"
        property int fw: backgroundItem.pixelMetric("menupanelwidth");
        anchors.leftMargin: backgroundItem.pixelMetric("menuhmargin") + fw
        anchors.rightMargin: backgroundItem.pixelMetric("menuhmargin") + fw
        anchors.topMargin: backgroundItem.pixelMetric("menuvmargin") + fw
        anchors.bottomMargin: backgroundItem.pixelMetric("menuvmargin") + fw
        elementType: "menu"

        effect: DropShadow {
            blurRadius: 18
            color: "#90000000"
            xOffset: 1
            yOffset: 1
        }
    }
}
