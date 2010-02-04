import Qt 4.6
import Bauhaus 1.0

QWidget { //This is a special CheckBox that does color coding for states

    id: CheckBox;

    property var backendValue;

    property var baseStateFlag;
    property alias checkable: LocalCheckBox.checkable
    property alias text: LocalLabel.text

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
            if (baseStateFlag) {
                if (backendValue != null && backendValue.isInModel)
                    LocalLabel.setStyleSheet("color: white;");
                else
                    LocalLabel.setStyleSheet("color: gray;");
            } else {
                if (backendValue != null && backendValue.isInSubState)
                    LocalLabel.setStyleSheet("color: #7799FF;");
                else
                    LocalLabel.setStyleSheet("color: gray;");
            }
        }
    }
		 


    layout: HorizontalLayout {
        spacing: 4

        QCheckBox {
            id: LocalCheckBox
            checkable: true;
            checked: (backendValue === undefined || backendValue === null) ? false : backendValue.value;
            onToggled: {
                backendValue.value = checked;
            }
        }

        QLabel {
            id: LocalLabel
            font.bold: true;
			alignment: "Qt::AlignLeft | Qt::AlignVCenter"
        }

    }


    ExtendedFunctionButton {
        backendValue: CheckBox.backendValue
        y: 4
        x: LocalCheckBox.x + 18;
    }
}

