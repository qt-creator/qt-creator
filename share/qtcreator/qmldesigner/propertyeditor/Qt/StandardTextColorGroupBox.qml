import Qt 4.6
import Bauhaus 1.0

GroupBox {
    id: standardTextColorGroupBox
	
	property bool showStyleColor: false
	property bool showSelectionColor: false
	property bool showSelectedTextColor: false


    caption: "Color"

    layout: VerticalLayout {
        ColorGroupBox {
            caption: "Text"
            finished: finishedNotify

            backendColor: backendValues.color
        }
        
        ColorGroupBox {
		    caption: "Style"
			visible: showStyleColor
            finished: finishedNotify

            backendColor: backendValues.styleColor
        }
		
		 ColorGroupBox {
            caption: "Selection"
			visible: showSelectionColor
            finished: finishedNotify

            backendColor: backendValues.selectionColor
        }
        
        ColorGroupBox {
		    visible: showSelectedTextColor
		    caption: "Selected Text"

            finished: finishedNotify

            backendColor: backendValues.selectedTextColor
        }

    }

}
