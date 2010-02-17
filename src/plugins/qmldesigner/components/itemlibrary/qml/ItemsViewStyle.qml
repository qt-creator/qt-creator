import Qt 4.6

Item {
    property string backgroundColor: "#707070"
    property string raisedBackgroundColor: "#e0e0e0"

    property string scrollbarBackgroundColor: "#505050"
    property string scrollbarHandleColor: "#303030"

    property string itemNameTextColor: "#FFFFFF"

    property string sectionTitleTextColor: "#f0f0f0"
    property string sectionTitleBackgroundColor: "#909090"

//    property string gridLineLighter: "#787878"
//    property string gridLineDarker: "#656565"
    property string gridLineLighter: "#808080"
    property string gridLineDarker: "#606060"

    property int sectionTitleHeight: 20
    property int sectionTitleSpacing: 2

    property int selectionSectionOffset: sectionTitleHeight + sectionTitleSpacing

    property int iconWidth: 32
    property int iconHeight: 32

    property int textWidth: 80
    property int textHeight: 15

    property int cellSpacing: 7
    property int cellMargin: 10

    property int cellWidth: textWidth + 2*cellMargin
    property int cellHeight:  itemLibraryIconHeight + textHeight + 2*cellMargin + cellSpacing
}

