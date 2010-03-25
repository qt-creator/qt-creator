import Qt 4.6
import Bauhaus 1.0

GroupBox {
    id: standardTextColorGroupBox

    property bool showStyleColor: false
    property bool showSelectionColor: false
    property bool showSelectedTextColor: false

    caption: qsTr("Color")

    layout: VerticalLayout {
        ColorGroupBox {
            caption: qsTr("Text")
            finished: finishedNotify

            backendColor: backendValues.color
        }
        
        ColorGroupBox {
            caption: qsTr("Style")
            visible: showStyleColor
            finished: finishedNotify

            backendColor: backendValues.styleColor
        }

        ColorGroupBox {
            caption: qsTr("Selection")
            visible: showSelectionColor
            finished: finishedNotify

            backendColor: backendValues.selectionColor
        }
        
        ColorGroupBox {
            visible: showSelectedTextColor
            caption: qsTr("Selected")

            finished: finishedNotify

            backendColor: backendValues.selectedTextColor
        }

    }

}
