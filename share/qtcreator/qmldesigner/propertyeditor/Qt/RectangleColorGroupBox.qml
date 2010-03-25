import Qt 4.6
import Bauhaus 1.0

GroupBox {
    id: rectangleColorGroupBox

    caption: qsTr("Colors")

    layout: VerticalLayout {        

        ColorGroupBox {
            caption: qsTr("Rectangle")
            finished: finishedNotify            
            backendColor: backendValues.color
        }        

        ColorGroupBox {
            caption: qsTr("Border")
            finished: finishedNotify

            backendColor: backendValues.border_color
        }

    }

}
