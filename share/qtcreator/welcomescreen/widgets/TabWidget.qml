import QtQuick 1.0

Item {
    id: tabWidget
    property alias model: contentRepeater.model

    Item {
        id: stack

        anchors.margins: 0
        width: parent.width
        height: parent.height

        Repeater {
            id: contentRepeater
            Loader {
                id: pageLoader
                clip: true
                opacity: index == tabWidget.current
                anchors.fill: parent
                anchors.margins: 4
                source: model.modelData.pageLocation

                onStatusChanged: {
                    if (pageLoader.status == Loader.Error) console.debug(source + ' failed to load')
                }
            }
        }
    }
}
