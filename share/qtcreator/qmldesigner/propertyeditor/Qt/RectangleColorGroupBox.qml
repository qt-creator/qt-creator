import Qt 4.6
import Bauhaus 1.0

GroupBox {
    id: rectangleColorGroupBox

    caption: "Colors"

    layout: VerticalLayout {

        ColorLabel {
            text: "    Color"
        }

        ColorGroupBox {

            finished: finishedNotify

            backendColor: backendValues.color
        }

        ColorLabel {
            text: "    Border Color"
        }

        ColorGroupBox {

            finished: finishedNotify

            backendColor: backendValues.border_color
        }

    }

}
