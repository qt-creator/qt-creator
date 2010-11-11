import QtQuick 1.0

FocusScope {
    id: root
    signal textChanged
    property alias text : input.text
    height:input.font.pixelSize*1.8
    BorderImage {
        anchors.fill: parent
        source: "img/lineedit.png"
        border.left: 5; border.top: 5
        border.right: 5; border.bottom: 5
        TextInput {
            id: input
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 4
            anchors.rightMargin: 4
            property string defaultText: "Click here to search the tutorials and howtos"
            color: "grey"
            text:  defaultText
            font.pointSize: 12
            clip: true
            onActiveFocusChanged: activeFocus ? state = 'active' : state = ''
            onTextChanged: if (text != defaultText) root.textChanged();
            states: [ State { name: "active"; PropertyChanges { target: input; color: "black"; text: "" } } ]
        }
    }
}
