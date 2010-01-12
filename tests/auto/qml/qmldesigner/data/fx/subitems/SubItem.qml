import Qt 4.6

Rectangle {
    property string text

    width : {toprect.width}
    height : 20
    color : "blue"
    Text {
        width : 100
        color : "white"
        text : parent.text
    }
}
