import Qt 4.7
import Bauhaus 1.0

//backendValue: backendValues.horizontalVelocity;
//backendValue: backendValues.verticalVelocity;
//backendValue: backendValues.maximumFlickVelocity;
//backendValue: backendValues.overShoot;

//boundsBehavior : enumeration
//contentHeight : int
//contentWidth : int

QWidget {
    layout: QVBoxLayout {
        topMargin: 0
        bottomMargin: 0
        leftMargin: 0
        rightMargin: 0
        spacing: 0        
            GroupBox {
                finished: finishedNotify;
                caption: qsTr("Flickable")                
                }
        }
    }
}