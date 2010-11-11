import QtQuick 1.0

BasicButton {
    id: button

    property string text
    property url iconSource
    property Component label: null

    // implementation

    background: defaultStyle.background
    property Item labelItem: labelLoader.item

    Loader {
        id: labelLoader
        anchors.fill: parent
        property alias styledItem: button
        sourceComponent: label
    }
}
