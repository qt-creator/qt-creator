import Qt 4.6
import Bauhaus 1.0

QWidget { //This is a special DoubleSpinBox that does color coding for states
    id: DoubleSpinBox;

    property var backendValue;
    property var baseStateFlag;
    property alias singleStep: box.singleStep
    property alias minimum: box.minimum
    property alias maximum: box.maximum
    property alias text: label.text
    property var enabled

    minimumHeight: 22;

    onBaseStateFlagChanged: {
        evaluate();
    }

    onBackendValueChanged: {
        evaluate();
    }

    Script {        
        function evaluate() {
            if (baseStateFlag) {
                if (backendValue != null && backendValue.isInModel)
                    box.setStyleSheet("color: white;");
                else
                    box.setStyleSheet("color: gray;");
            } else {
                if (backendValue != null && backendValue.isInSubState)
                    box.setStyleSheet("color: #7799FF;");
                else
                    box.setStyleSheet("color: gray;");
            }
        }
    }

    property bool isInModel: backendValue.isInModel

    onIsInModelChanged: {
        evaluate();
    }

    property bool isInSubState: backendValue.isInSubState

    onIsInSubStateChanged: {
        evaluate();
    }

    layout: HorizontalLayout {        

        QLabel {
            visible: text != ""
            id: label;
            font.bold: true;
            alignment: "Qt::AlignRight | Qt::AlignVCenter"
            maximumWidth: 80
        }

        QDoubleSpinBox {
            id: box;
            decimals: 1;
            keyboardTracking: false;
            enabled: !backendValue.isBound && DoubleSpinBox.enabled;
            property bool readingFromBackend: false;
            property real valueFromBackend: DoubleSpinBox.backendValue == null ? .0 : DoubleSpinBox.backendValue.value;
            
            onValueFromBackendChanged: {
                readingFromBackend = true;
                value = valueFromBackend
                readingFromBackend = false;
            }           

            onValueChanged: {
                if (DoubleSpinBox.backendValue != null && readingFromBackend == false)
                    DoubleSpinBox.backendValue.value = value;
            }

            onMouseOverChanged: {

            }

        }

    }

    ExtendedFunctionButton {
        backendValue: DoubleSpinBox.backendValue
        y: box.y + 4
        x: box.x + 2
        visible: DoubleSpinBox.enabled
    }
}
