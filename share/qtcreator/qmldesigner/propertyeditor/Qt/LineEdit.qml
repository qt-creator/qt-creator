import Qt 4.7
import Bauhaus 1.0

QWidget {
    id: lineEdit

    property variant backendValue
    property alias enabled: lineEdit.enabled
    property variant baseStateFlag
    property alias text: lineEditWidget.text
    property alias readOnly: lineEditWidget.readOnly
    property alias translation: trCheckbox.visible

    minimumHeight: 24;

    onBaseStateFlagChanged: {
        evaluate();
    }

    property variant isEnabled: lineEdit.enabled
    onIsEnabledChanged: {
        evaluate();
    }


    property bool isInModel: backendValue.isInModel;
    onIsInModelChanged: {
        evaluate();
    }
    property bool isInSubState: backendValue.isInSubState;
    onIsInSubStateChanged: {
        evaluate();
    }

    function evaluate() {
        if (!enabled) {
            lineEditWidget.setStyleSheet("color: "+scheme.disabledColor);
        } else {
            if (baseStateFlag) {
                if (backendValue != null && backendValue.isInModel)
                    lineEditWidget.setStyleSheet("color: "+scheme.changedBaseColor);
                else
                    lineEditWidget.setStyleSheet("color: "+scheme.defaultColor);
            } else {
                if (backendValue != null && backendValue.isInSubState)
                    lineEditWidget.setStyleSheet("color: "+scheme.changedStateColor);
                else
                    lineEditWidget.setStyleSheet("color: "+scheme.defaultColor);
            }
        }
    }

    ColorScheme { id:scheme; }

    QLineEdit {
        y: 2
        id: lineEditWidget
        styleSheet: "QLineEdit { padding-left: 32; }"
        width: lineEdit.width
        height: lineEdit.height
        toolTip: backendValue.isBound ? backendValue.expression : ""
        
        property string valueFromBackend: (backendValue === undefined || backendValue.value === undefined) ? "" : backendValue.value;

        onValueFromBackendChanged: {
            if (backendValue.value === undefined)
                return;
            text = backendValue.value;
        }

        onEditingFinished: {
            if (backendValue.isTranslated) {
                backendValue.expression = "qsTr(\"" + text + "\")"
            } else {
                backendValue.value = text
            }
            evaluate();
        }

        onFocusChanged: {
            if (focus)
                backendValue.lock();
            else
                backendValue.unlock();
        }


    }
    ExtendedFunctionButton {
        backendValue: lineEdit.backendValue
        y: 6
        x: 0
        visible: lineEdit.enabled
    }
    QCheckBox {
        id: trCheckbox
        y: 2
        styleSheetFile: "checkbox_tr.css";
        toolTip: qsTr("Translate this string")
        x: lineEditWidget.width - 22
        height: lineEdit.height - 2;
        width: 24
        visible: false
        checked: backendValue.isTranslated
        onToggled: {
            if (trCheckbox.checked) {
                backendValue.expression = "qsTr(\"" + lineEditWidget.text + "\")"
            } else {
                backendValue.value = lineEditWidget.text
            }
            evaluate();
        }

        onVisibleChanged: {
            if (trCheckbox.visible) {
                trCheckbox.raise();
                lineEditWidget.styleSheet =  "QLineEdit { padding-left: 32; padding-right: 62;}"
            }
        }
    }
}
