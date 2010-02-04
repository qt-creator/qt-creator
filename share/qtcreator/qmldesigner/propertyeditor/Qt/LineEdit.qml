import Qt 4.6
import Bauhaus 1.0

QWidget {
    id: LineEdit

    property var backendValue
    property alias enabled: LineEdit.enabled

    minimumHeight: 24;

    QLineEdit {
        id: lineEditWidget
		styleSheet: "padding-left: 16;"
        width: LineEdit.width
        height: LineEdit.height

        text: backendValue.value

        onTextEdited: {
            backendValue.value = text
        }


    }    
    ExtendedFunctionButton {
        backendValue: LineEdit.backendValue
        y: 4
        x: 3
        visible: LineEdit.enabled
    }
}
