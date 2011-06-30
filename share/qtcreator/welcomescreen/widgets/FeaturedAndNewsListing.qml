import QtQuick 1.0
import components 1.0 as Components

Item {
    InsetText {
        id: text
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top:  parent.top
        anchors.margins: 10
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("Featured News")
//        mainColor: "#44A51C"
        mainColor: "#074C1C"
        font.bold: true
        font.pointSize: 16
    }

    ListModel {
        id: tempNewsModel
        ListElement { title: ""; description: "Loading news sources..." ; blogIcon: ""; blogName: ""; link: "" }
    }

    NewsListing {
        id: newsList
        model: {
            if (aggregatedFeedsModel.articleCount > 0)
                return aggregatedFeedsModel
            else
                return tempNewsModel
        }
        anchors.bottom: parent.bottom
        anchors.top: text.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: text.height
        clip: true

    }

}
