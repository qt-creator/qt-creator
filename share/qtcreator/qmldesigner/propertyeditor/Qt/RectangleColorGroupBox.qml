import Qt 4.6
import Bauhaus 1.0

GroupBox {
    id: rectangleColorGroupBox

    caption: "Colors"

    layout: VerticalLayout {        

        ColorGroupBox {
		    caption: "Rectangle"

            finished: finishedNotify

            backendColor: backendValues.color
        }        

        ColorGroupBox {
            caption: "Border"
            finished: finishedNotify

            backendColor: backendValues.border_color
        }

    }

}
