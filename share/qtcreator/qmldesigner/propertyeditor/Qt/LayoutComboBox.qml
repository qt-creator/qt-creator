import Qt 4.6
import Bauhaus 1.0

QWidget {
    id: layoutComboBox
    property alias itemNode: comboBox.itemNode
    property alias selectedItemNode: comboBox.selectedItemNode

    property alias enabled: comboBox.enabled

    property var isEnabled: comboBox.enabled
    onIsEnabledChanged: {
        evaluate();
    }

    // special:  will only be valid in base state
    Script {
        function evaluate() {
            if (!enabled) {
                comboBox.setStyleSheet("color: "+scheme.disabledColor);
            } else {
                comboBox.setStyleSheet("QComboBox,QComboBox:on{color: "+scheme.defaultColor+"}QComboBox:off{color:"+scheme.optionsColor+"}");
            }
        }
    }

    ColorScheme { id:scheme; }


    layout: HorizontalLayout {
        SiblingComboBox {
            id: comboBox
        }
    }
}
