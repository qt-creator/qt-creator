import Qt 4.6
import Bauhaus 1.0

QWidget {

    id: comboBox

    property var backendValue;
    property var baseStateFlag;
    property alias enabled: box.enabled;

    property alias items: box.items;
    property alias currentText: box.currentText;


    onBaseStateFlagChanged: {
        evaluate();
    }

    property var isEnabled: comboBox.enabled
    onIsEnabledChanged: {
        evaluate();
    }

    Script {
        function evaluate() {
		    if (backendValue === undefined)
		        return;
            if (!enabled) {
                box.setStyleSheet("color: "+scheme.disabledColor);
            } else {
                if (baseStateFlag) {
                    if (backendValue.isInModel)
                        box.setStyleSheet("QComboBox,QComboBox:on{color: "+scheme.changedBaseColor+"}QComboBox:off{color:"+scheme.optionsColor+"}");
                    else
                        box.setStyleSheet("QComboBox,QComboBox:on{color: "+scheme.defaultColor+"}QComboBox:off{color:"+scheme.optionsColor+"}");
                    } else {
                    if (backendValue.isInSubState)
                        box.setStyleSheet("QComboBox,QComboBox:on{color: "+scheme.changedStateColor+"}QComboBox:off{color:"+scheme.optionsColor+"}");
                    else
                        box.setStyleSheet("QComboBox,QComboBox:on{color: "+scheme.defaultColor+"}QComboBox:off{color:"+scheme.optionsColor+"}");
                    }
            }

        }
    }

    ColorScheme { id:scheme; }

    layout: HorizontalLayout {
        QComboBox {
            id: box
            property var backendValue: comboBox.backendValue
            onCurrentTextChanged: { backendValue.value = currentText; evaluate(); }
            ExtendedFunctionButton {
                backendValue: comboBox.backendValue;
                y: 3
                x: 3
            }
        }
    }
}
