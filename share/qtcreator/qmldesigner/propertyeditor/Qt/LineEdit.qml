import Qt 4.6
import Bauhaus 1.0

QWidget {
    id: lineEdit

    property var backendValue
    property alias enabled: lineEdit.enabled

    minimumHeight: 24;

    QLineEdit {
        id: lineEditWidget
		styleSheet: "padding-left: 16;"
        width: lineEdit.width
        height: lineEdit.height

        text: backendValue.value

        onTextEdited: {
            backendValue.value = text
        }
		
		onFocusChanged: {				
			if (focus)
			    backendValue.lock();
			else
			    backendValue.unlock();
		}


    }    
    ExtendedFunctionButton {
        backendValue: lineEdit.backendValue
        y: 4
        x: 3
        visible: lineEdit.enabled
    }
}
