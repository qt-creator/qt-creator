import QtQuick 1.0
import com.nokia.symbian 1.0

ListView {
    x: 0
    y: 36
    width: 360
    height: 320
    clip: true
    header: ListHeading {
        ListItemText {
            anchors.fill: parent.paddingItem
            role: "Heading"
            text: "ListHeading"
        }
    }
    delegate:    ListItem {
        id: listItem
        Column {
            anchors.fill: parent.paddingItem
            ListItemText {
                width: parent.width
                mode: listItem.mode
                role: "Title"
                text: titleText
            }
            ListItemText {
                width: parent.width
                mode: listItem.mode
                role: "SubTitle"
                text: subTitleText
            }
        }
    }
    model: ListModel {
        ListElement {
            titleText: "Title1"
            subTitleText: "SubTitle1"
        }
        ListElement {
            titleText: "Title2"
            subTitleText: "SubTitle2"
        }
        ListElement {
            titleText: "Title3"
            subTitleText: "SubTitle3"
        }
        ListElement {
            titleText: "Title4"
            subTitleText: "SubTitle4"
        }
    }
}
