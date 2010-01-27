import Qt 4.6
import Bauhaus 1.0

QComboBox {
	id: ComboBox
	
	property var backendValue
	
	ExtendedFunctionButton {
        backendValue: ComboBox.backendValue
        y: 3
        x: 3
        visible: CheckBox.enabled
    }
}