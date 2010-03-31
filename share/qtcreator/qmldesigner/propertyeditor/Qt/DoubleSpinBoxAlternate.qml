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

    onEnabledChanged: {
        evaluate();
    }

    function evaluate() {
        if (backendValue === undefined)
            return;
        if (!enabled) {
            box.setStyleSheet("color: "+scheme.disabledColor);
        } else {
            if (baseStateFlag) {
                if (backendValue.isInModel)
                    box.setStyleSheet("color: "+scheme.changedBaseColor);
                else
                    box.setStyleSheet("color: "+scheme.defaultColor);
            } else {
                if (backendValue.isInSubState)
                    box.setStyleSheet("color: "+scheme.changedStateColor);
                else
                    box.setStyleSheet("color: "+scheme.defaultColor);
            }
        }
    }

    ColorScheme { id:scheme; }

    property bool isInModel: backendValue.isInModel;

    onIsInModelChanged: {
        evaluate();
    }

    property bool isInSubState: backendValue.isInSubState;

    onIsInSubStateChanged: {
        evaluate();
    }

    layout: HorizontalLayout {
        id: layoutH;

        QLabel {
            id: label;
            font.bold: true;
            alignment: doubleSpinBox.alignRight  ? "Qt::AlignRight | Qt::AlignVCenter" : "Qt::AlignLeft | Qt::AlignVCenter";
            fixedWidth: 68
            visible: doubleSpinBox.text != "";
        }

        QDoubleSpinBox {
            id: box;
            decimals: 1;
            keyboardTracking: false;
            enabled: !backendValue.isBound && doubleSpinBox.enabled;

            property bool readingFromBackend: false;
            property real valueFromBackend: doubleSpinBox.backendValue.value;

            onValueFromBackendChanged: {
                readingFromBackend = true;
                value = valueFromBackend
                readingFromBackend = false;
            }

            onValueChanged: {
                doubleSpinBox.backendValue.value = value;
            }

            onMouseOverChanged: {

            }

            onFocusChanged: {
                if (focus)
                transaction.start();
                else
                transaction.end();
            }
        }
    }

    ExtendedFunctionButton {
        backendValue: doubleSpinBox.backendValue;
        y: box.y + 4
        x: box.x + 2
        visible: doubleSpinBox.enabled
    }
}
