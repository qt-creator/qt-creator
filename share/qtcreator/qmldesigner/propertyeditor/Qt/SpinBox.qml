import Qt 4.6
import Bauhaus 1.0

QWidget { //This is a special spinBox that does color coding for states

    id: spinBox;

    property var backendValue;
    property var baseStateFlag;
    property alias singleStep: box.singleStep;
    property alias minimum: box.minimum
    property alias maximum: box.maximum
    property alias enabled: spinBox.enabled

    minimumHeight: 22;

    onBaseStateFlagChanged: {
        evaluate();
    }

    onBackendValueChanged: {
        evaluate();
    }

    property var isEnabled: spinBox.enabled
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
                box.setStyleSheet("color: "+scheme.changedBaseColor);
            else
                box.setStyleSheet("color: "+scheme.defaultColor);
            } else {
            if (backendValue != null && backendValue.isInSubState)
                box.setStyleSheet("color: "+scheme.changedStateColor);
            else
                box.setStyleSheet("color: "+scheme.defaultColor);
            }
        }
    }
    }
	
	property bool isInModel: (backendValue === undefined || backendValue === null) ? false: backendValue.isInModel;

    onIsInModelChanged: {
        evaluate();
    }

    property bool isInSubState: (backendValue === undefined || backendValue === null) ? false: backendValue.isInSubState;

    onIsInSubStateChanged: {
        evaluate();
    }

    ColorScheme { id:scheme; }

    layout: HorizontalLayout {

        QSpinBox {
            property alias backendValue: spinBox.backendValue

            keyboardTracking: false;
            id: box;
            enabled: backendValue === undefined || backendValue.isBound === undefined || backendValue.isBound === null ? false : !backendValue.isBound
            property bool readingFromBackend: false;
            property int valueFromBackend: (spinBox.backendValue === undefined || spinBox.backendValue == null  || spinBox.backendValue.value === undefined)
            ? .0 : spinBox.backendValue.value;

            onValueFromBackendChanged: {
                readingFromBackend = true;
				if (!(valueFromBackend  === undefined))
                    value = valueFromBackend;
                readingFromBackend = false;
                evaluate();
            }

            onValueChanged: {
                if (spinBox.backendValue != null && readingFromBackend == false)
                    backendValue.value = value;
            }

            onFocusChanged: {
                if (focus)
                    spinBox.backendValue.lock();
                else
                    spinBox.backendValue.unlock();
            }
            onEditingFinished: {
                focus = false;
            }
        }
    }

    ExtendedFunctionButton {
        backendValue: (spinBox.backendValue === undefined ||
        spinBox.backendValue === null)
        ? null : spinBox.backendValue;
        y: box.y + 4
        x: box.x + 2
        visible: spinBox.enabled
    }
}
