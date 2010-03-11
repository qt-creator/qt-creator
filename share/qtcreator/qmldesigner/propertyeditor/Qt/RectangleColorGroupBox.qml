import Qt 4.6
import Bauhaus 1.0

GroupBox {
    id: rectangleColorGroupBox

    caption: "Colors"

    layout: VerticalLayout {        

        ColorGroupBox {
		    caption: "Color"

            finished: finishedNotify

            backendColor: backendValues.color
        }        

        ColorGroupBox {
            caption: "Border Color"
            finished: finishedNotify

            backendColor: backendValues.border_color
        }

    }

}
