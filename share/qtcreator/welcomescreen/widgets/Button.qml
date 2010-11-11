import Qt 4.7

BorderImage {
    property string text
    property string image
    property int iconSize : innerImg.sourceSize.height
    signal clicked;
    id: root
    source: "qrc:/welcome/images/btn_26.png"
    height: innerImg.height + 16

    border.left: 5; border.top: 5
    border.right: 5; border.bottom: 5

    Image{
        id: innerImg
        height: root.iconSize
        width: root.iconSize
        anchors.verticalCenter: label.verticalCenter
        anchors.right: label.left
        anchors.rightMargin: 4
        visible: root.image != ""
        source: root.image
    }

    Text {
        id: label;
        anchors.left: innerImg.right
        anchors.right: parent.right
        text: root.text
    }

    MouseArea { id: mouseArea; anchors.fill: parent; hoverEnabled: true; onClicked: root.clicked() }

    states: [
        State {
            id: pressedState; when: mouseArea.pressed;
            PropertyChanges { target: root; source: "qrc:/welcome/images/btn_26_pressed.png" }
        },
        State {
            id: hoverState; when: mouseArea.containsMouse
            PropertyChanges { target: root; source: "qrc:/welcome/images/btn_26_hover.png" }
        }
    ]
}
