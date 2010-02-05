import Qt 4.6
import Bauhaus 1.0

QWidget { //This is a special doubleSpinBox that does color coding for states
    id: doubleSpinBox;

    property var backendValue;
    property var baseStateFlag;
    property alias singleStep: box.singleStep
    property alias minimum: box.minimum
    property alias maximum: box.maximum
	property alias spacing: layoutH.spacing
    property alias text: label.text
	property bool alignRight: true
    property bool enabled: true

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

    property bool isInModel: (backendValue === undefined || backendValue === null) ? false: backendValue.isInModel;

    onIsInModelChanged: {
        evaluate();
    }

    property bool isInSubState: (backendValue === undefined || backendValue === null) ? false: backendValue.isInSubState;

    onIsInSubStateChanged: {
        evaluate();
    }

    layout: HorizontalLayout {
		id: layoutH;

        QLabel {
            id: label;
            font.bold: true;
            alignment: doubleSpinBox.alignRight  ? "Qt::AlignRight | Qt::AlignVCenter" : "Qt::AlignLeft | Qt::AlignVCenter";
            maximumWidth: 40
            visible: doubleSpinBox.text != "";
        }

        QDoubleSpinBox {
            id: box;
            decimals: 1;
            keyboardTracking: false;
            enabled: (doubleSpinBox.backendValue === undefined ||
                      doubleSpinBox.backendValue === null)
            ? true : !backendValue.isBound && doubleSpinBox.enabled;

            property bool readingFromBackend: false;
            property real valueFromBackend: (doubleSpinBox.backendValue === undefined ||
                                             doubleSpinBox.backendValue === null || doubleSpinBox.backendValue.value === undefined)
            ? .0 : doubleSpinBox.backendValue.value;
            
            onValueFromBackendChanged: {
                readingFromBackend = true;
                value = valueFromBackend
                readingFromBackend = false;
            }           

            onValueChanged: {
                if (doubleSpinBox.backendValue != null && readingFromBackend == false)
                    doubleSpinBox.backendValue.value = value;
            }

            onMouseOverChanged: {

            }
        }
    }

    ExtendedFunctionButton {
        backendValue: (doubleSpinBox.backendValue === undefined ||
                       doubleSpinBox.backendValue === null)
             ? null : doubleSpinBox.backendValue;
        y: box.y + 4
        x: box.x + 2
        visible: doubleSpinBox.enabled
    }
}
