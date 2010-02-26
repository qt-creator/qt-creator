import Qt 4.6
import Bauhaus 1.0

QWidget { //This is a special checkBox that does color coding for states

    id: checkBox;

    property var backendValue;

    property var baseStateFlag;
    property alias checkable: localCheckBox.checkable
    property alias text: localLabel.text

    onBaseStateFlagChanged: {
        evaluate();
    }

    onBackendValueChanged: {
        evaluate();
    }

    property bool isInModel: (backendValue === undefined || backendValue === null) ? false: backendValue.isInModel;

    onIsInModelChanged: {
        evaluate();
    }

    property bool isInSubState: (backendValue === undefined || backendValue === null) ? false: backendValue.isInSubState;

    onIsInSubStateChanged: {
        evaluate();
    }

    Script {
        function evaluate() {
            if (!enabled) {
                localLabel.setStyleSheet("color: "+scheme.disabledColor);
            } else {
                if (baseStateFlag) {
                    if (backendValue != null && backendValue.isInModel)
                        localLabel.setStyleSheet("color: "+scheme.changedBaseColor);
                    else
                        localLabel.setStyleSheet("color: "+scheme.boldTextColor);
                } else {
                    if (backendValue != null && backendValue.isInSubState)
                        localLabel.setStyleSheet("color: "+scheme.changedStateColor);
                    else
                        localLabel.setStyleSheet("color: "+scheme.boldTextColor);
                }
            }
        }
    }

    ColorScheme { id:scheme; }


    layout: HorizontalLayout {
        spacing: 4

        QCheckBox {
            id: localCheckBox
            checkable: true;
            checked: (backendValue === undefined || backendValue === null) ? false : backendValue.value;
            onToggled: {
                backendValue.value = checked;
            }
            maximumWidth: 30
        }

        QLabel {
            id: localLabel
            font.bold: true;
            alignment: "Qt::AlignLeft | Qt::AlignVCenter"
        }

    }


    ExtendedFunctionButton {
        backendValue: checkBox.backendValue
        y: 4
        x: localCheckBox.x + 18;
    }
}

