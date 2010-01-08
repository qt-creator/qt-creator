import Qt 4.6
import Bauhaus 1.0

QWidget { //This is a special SpinBox that does color coding for states
    id: SpinBox;

    property var backendValue;
    property var baseStateFlag;
    property alias singleStep: box.singleStep;
    property alias minimum: box.minimum
    property alias maximum: box.maximum

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
                    SpinBox.setStyleSheet("color: white;");
                else
                    SpinBox.setStyleSheet("color: gray;");
            } else {
                if (backendValue != null && backendValue.isInSubState)
                    SpinBox.setStyleSheet("color: #7799FF;");
                else
                    SpinBox.setStyleSheet("color: gray;");
            }
        }
    }

    layout: QHBoxLayout {
        topMargin: 0;
        bottomMargin: 0;
        leftMargin: 0;
        rightMargin: 10;
        spacing: 0;

        QSpinBox {
            property alias backendValue: SpinBox.backendValue

            keyboardTracking: false;
            id: box;
            enabled: backendValue === undefined || backendValue.isBound === undefined || backendValue.isBound === null ? false : !backendValue.isBound
            value: backendValue == undefined || backendValue.value == undefined || backendValue.value === null ? 0 : backendValue.value;
            onValueChanged: {
                if (backendValue != undefined && backendValue != null)
                    backendValue.value = value;
            }
            onFocusChanged: {
                //extendedSwitches.active = focus;
                //extendedSwitches.backendValue = backendValue;
            }
        }
    }

     QToolButton {
        visible: false;
        width: 10;
        height: 10;
        y: box.y + box.height - 11;
        x: box.width - 1;
        focusPolicy: "Qt::NoFocus";
    }
}
