import Qt 4.6
import Bauhaus 1.0

QWidget {

    id: comboBox

    property var backendValue
    property var baseStateFlag;
    property alias enabled: comboBox.enabled

    property var items
    property var currentText


    onBaseStateFlagChanged: {
        evaluate();
    }

    onBackendValueChanged: {
        evaluate();
    }

    property var isEnabled: comboBox.enabled
    onIsEnabledChanged: {
        evaluate();
    }



    Script {
        function evaluate() {
            if (!enabled) {
                box.setStyleSheet("color: "+scheme.disabledColor);
            } else {
                if (baseStateFlag) {
                    if (backendValue != null && backendValue.isInModel)
                        box.setStyleSheet("QComboBox{color: "+scheme.changedBaseColor+"}QComboBox:on{color:white}");
                    else
                        box.setStyleSheet("QComboBox{color: "+scheme.defaultColor+"}QComboBox:on{color:white}");
                    } else {
                    if (backendValue != null && backendValue.isInSubState)
                        box.setStyleSheet("QComboBox{color: "+scheme.changedStateColor+"}QComboBox:on{color:white}");
                    else
                        box.setStyleSheet("QComboBox{color: "+scheme.defaultColor+"}QComboBox:on{color:white}");
                    }
            }
        }
    }

    ColorScheme { id:scheme; }

    layout: HorizontalLayout {
        QComboBox {
            id: box
            property var backendValue: comboBox.backendValue
            items: comboBox.items
            currentText: comboBox.currentText
            ExtendedFunctionButton {
                backendValue: (comboBox.backendValue === undefined || comboBox.backendValue === null)
                ? null : comboBox.backendValue;
                y: 3
                x: 3
            }
        }
    }
}
