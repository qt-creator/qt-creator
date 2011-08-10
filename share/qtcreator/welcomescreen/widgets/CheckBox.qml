import QtQuick 1.0
import qtcomponents.custom 1.0 as Custom

Custom.CheckBox{
    id:checkbox
    property string text
    property string hint
    height:20
    width: Math.max(110, backgroundItem.rowWidth)

    background: Item {
        property int rowWidth: row.width
        Row {
            id: row
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4
            BorderImage {
                source: "qrc:welcome/images/lineedit.png";
                width: 16;
                height: 16;
                border.left: 4;
                border.right: 4;
                border.top: 4;
                border.bottom: 4
                Image {
                    source: "qrc:welcome/images/checked.png";
                    width: 10; height: 10
                    anchors.centerIn: parent
                    visible: checkbox.checked
                }
            }
            Text {
                text: checkbox.text
            }
        }
        Keys.onSpacePressed:checked = !checked

    }
}
