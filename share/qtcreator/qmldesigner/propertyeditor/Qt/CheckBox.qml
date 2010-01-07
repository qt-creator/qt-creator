import Qt 4.6

QCheckBox { //This is a special CheckBox that does color coding for states
   id: CheckBox;

   property var backendValue;
   property var baseStateFlag;

   checkable: true;
   checked: backendValue.value === undefined ? false : backendValue.value;
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
               if (backendValue.isInModel)
                    CheckBox.setStyleSheet("color: white;");
               else
                   CheckBox.setStyleSheet("color: gray;");
           } else {
               if (backendValue.IsInSubState)
                   CheckBox.setStyleSheet("color: blue;");
               else
                   CheckBox.setStyleSheet("color: gray;");
          }
       }
   }
}
