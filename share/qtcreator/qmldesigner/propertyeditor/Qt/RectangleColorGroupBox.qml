import Qt 4.6
import Bauhaus 1.0

GroupBox {
		id: RectangleColorGroupBox

            finished: finishedNotify;
            caption: "Colors"
			
layout: VerticalLayout {
				
        ColorLabel {
            text: "    Color"			
        }

        ColorTypeButtons {

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