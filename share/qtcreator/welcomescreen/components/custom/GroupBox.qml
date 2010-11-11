import QtQuick 1.0

FocusScope {
    id: groupbox

    width: Math.max(200, contentWidth + loader.leftMargin + loader.rightMargin)
    height: contentHeight + loader.topMargin + loader.bottomMargin

    default property alias children: content.children

    property string title
    property bool checkable: false
    property int contentWidth: content.childrenRect.width
    property int contentHeight: content.childrenRect.height
    property double contentOpacity: 1

    property Component background: null
    property Item backgroundItem: loader.item

    property CheckBox checkbox: check
    property alias checked: check.checked

    Loader {
        id: loader
        anchors.fill: parent
        property int topMargin: 22
        property int bottomMargin: 4
        property int leftMargin: 4
        property int rightMargin: 4

        property alias styledItem: groupbox
        sourceComponent: background

        Item {
            id:content
            z: 1
            opacity: contentOpacity
            anchors.topMargin: loader.topMargin
            anchors.leftMargin: 8
            anchors.top:parent.top
            anchors.left:parent.left
            enabled: (!checkable || checkbox.checked)
        }

        CheckBox {
            id: check
            checked: true
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: loader.topMargin
        }
    }
}
