import Qt 4.6

QCheckBox { //This is a special CheckBox that does color coding for states
   id: CheckBox;

   property var backendValue;
   property var baseStateFlag;

   checkable: true;
   checked: (backendValue === undefined || backendValue === null) ? false : backendValue.value;
   onToggled: {
       backendValue.value = checked;
   }

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
                    CheckBox.setStyleSheet("color: white;");
                else
                    CheckBox.setStyleSheet("color: gray;");
            } else {
                if (backendValue != null && backendValue.isInSubState)
                    CheckBox.setStyleSheet("color: #7799FF;");
                else
                    CheckBox.setStyleSheet("color: gray;");
            }
		}
   }
   
   ExtendedFunctionButton {
        backendValue: CheckBox.backendValue
        y: 2
        x: 0
    }
}
