import Qt 4.6

Item {
    height: 100
    id: Root
    signal engineClicked(int id)
    signal refreshEngines()

    Row {
        anchors.fill: parent
        Repeater {
            model: engines
            Item {
                width: 100; height: 100;
                Image { 
                    id: EngineIcon; 
                    source: "qrc:/engine.png"
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text { 
                    anchors.top: EngineIcon.bottom; 
                    text: modelData.name + "(" + modelData.engineId + ")"
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                MouseRegion {
                    anchors.fill: parent
                    onClicked: Root.engineClicked(modelData.engineId);
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
        MouseRegion {
            anchors.fill: parent
            onClicked: Root.refreshEngines()
        }
    }
}
