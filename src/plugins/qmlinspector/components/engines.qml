import Qt 4.6

Item {
    height: 100
    id: root
    signal engineClicked(int id)
    signal refreshEngines()

    Row {
        anchors.fill: parent
        Repeater {
            model: engines
            Item {
                width: 100; height: 100;
                Image { 
                    id: engineIcon; 
                    source: "qrc:/engine.png"
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text { 
                    anchors.top: engineIcon.bottom; 
                    text: modelData.name + "(" + modelData.engineId + ")"
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: root.engineClicked(modelData.engineId);
                }
            }
        }
    }


    Image { 
        y: 15
        source: "qrc:/refresh.png";
        width: 75; 
        height: 63; 
        smooth: true 
        anchors.right: parent.right
        MouseArea {
            anchors.fill: parent
            onClicked: Root.refreshEngines()
        }
    }
}
