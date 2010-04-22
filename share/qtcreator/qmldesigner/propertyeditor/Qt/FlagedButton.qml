import Qt 4.7
import Bauhaus 1.0

QPushButton {
    id: flagedButton
    property bool flagActive: false
    property variant backendValue    
    property variant theValue: backendValue.value;   
    property bool blueHigh: false    
    property bool baseStateFlag: isBaseState;
    
    onBaseStateFlagChanged: {
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
    
    onTheValueChanged: {        
        evaluate();
    }
    
     function evaluate() {
        if (!enabled) {
            fontSelector.setStyleSheet("color: "+scheme.disabledColor);
        } else {
            if (baseStateFlag) {
                if (backendValue != null && backendValue.isInModel)
                    flagActive = true;
                else
                    flagActive = false;
            } else {
                if (backendValue != null && backendValue.isInSubState)
                    flagActive = true;
                else
                    flagActive = false;
            }
        }
    }
}