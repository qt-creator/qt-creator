import Qt 4.6
import Bauhaus 1.0

QComboBox {
	id: comboBox
	
	property var backendValue
	
        ExtendedFunctionButton {
            backendValue: (comboBox.backendValue === undefined || comboBox.backendValue === null)
            ? null : comboBox.backendValue;
            y: 3
            x: 3
        }
}
