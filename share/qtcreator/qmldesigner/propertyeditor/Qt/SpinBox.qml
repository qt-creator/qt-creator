import Qt 4.6
import Bauhaus 1.0

QWidget { //This is a special SpinBox that does color coding for states
    id: SpinBox;

    property var backendValue;
    property var baseStateFlag;
    property alias singleStep: box.singleStep;
    property alias minimum: box.minimum
    property alias maximum: box.maximum
    property alias enabled: SpinBox.enabled

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

    layout: HorizontalLayout {
        
        QSpinBox {
            property alias backendValue: SpinBox.backendValue

            keyboardTracking: false;
            id: box;
            enabled: backendValue === undefined || backendValue.isBound === undefined || backendValue.isBound === null ? false : !backendValue.isBound
            property bool readingFromBackend: false;
            property int valueFromBackend: SpinBox.backendValue == null ? .0 : SpinBox.backendValue.value;

            onValueFromBackendChanged: {
                readingFromBackend = true;
                value = valueFromBackend
                readingFromBackend = false;
                evaluate();
            }

            onValueChanged: {
                if (SpinBox.backendValue != null && readingFromBackend == false)
                    backendValue.value = value;
            }
            onFocusChanged: {
                //extendedSwitches.active = focus;
                //extendedSwitches.backendValue = backendValue;
            }
        }
    }

    ExtendedFunctionButton {        
        backendValue: (SpinBox.backendValue === undefined ||
                       SpinBox.backendValue === null)
        ? null : SpinBox.backendValue;
        y: box.y + 4
        x: box.x + 2
        visible: SpinBox.enabled
    }
}
