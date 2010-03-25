import Qt 4.6
import Bauhaus 1.0

QWidget {
    id: fontComboBox

    property alias currentFont: fontSelector.currentFont
    property var backendValue
    property var baseStateFlag;
    property alias enabled: fontSelector.enabled

    onBaseStateFlagChanged: {
        evaluate();
    }

    onBackendValueChanged: {
        evaluate();
    }

    property var isEnabled: fontComboBox.enabled
    onIsEnabledChanged: {
        evaluate();
    }

    function evaluate() {
        if (!enabled) {
            fontSelector.setStyleSheet("color: "+scheme.disabledColor);
        } else {
            if (baseStateFlag) {
                if (backendValue != null && backendValue.isInModel)
                    fontSelector.setStyleSheet("QComboBox,QComboBox:on{color: "+scheme.changedBaseColor+"}QComboBox:off{color:"+scheme.optionsColor+"}");
                else
                    fontSelector.setStyleSheet("QComboBox,QComboBox:on{color: "+scheme.defaultColor+"}QComboBox:off{color:"+scheme.optionsColor+"}");
            } else {
                if (backendValue != null && backendValue.isInSubState)
                    fontSelector.setStyleSheet("QComboBox,QComboBox:on{color: "+scheme.changedStateColor+"}QComboBox:off{color:"+scheme.optionsColor+"}");
                else
                    fontSelector.setStyleSheet("QComboBox,QComboBox:on{color: "+scheme.defaultColor+"}QComboBox:off{color:"+scheme.optionsColor+"}");
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
        QFontComboBox {
            id: fontSelector            
			
			currentFont.family: backendValue.value
			property var fontFamily: currentFont.family
            onFontFamilyChanged: {
			    if (backendValue === undefined)
			        return;
                if (backendValue.value != currentFont.family)
					backendValue.value = currentFont.family;
                }
        }
    }
}
