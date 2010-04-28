import Qt 4.7
import Bauhaus 1.0

QGroupBox {
    id: aligmentHorizontalButtons

    property variant theValue: backendValues.horizontalAlignment.value;

    property bool blueHigh: false

    property bool baseStateFlag: isBaseState;

    property variant backendValue: backendValues.horizontalAlignment;


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
        if (theValue != undefined) {
            leftButton.checked = theValue == "AlignLeft";
            centerButton.checked = theValue == "AlignHCenter";
            rightButton.checked = theValue == "AlignRight";
        }
        evaluate();
    }

    function evaluate() {
        if (!enabled) {
            fontSelector.setStyleSheet("color: "+scheme.disabledColor);
        } else {
            if (baseStateFlag) {
                if (backendValue != null && backendValue.isInModel)
                    blueHigh = true;
                else
                    blueHigh = false;
            } else {
                if (backendValue != null && backendValue.isInSubState)
                    blueHigh = true;
                else
                    blueHigh = false;
            }
        }
    }

    layout: HorizontalLayout {
        QWidget {
            fixedHeight: 32

            QPushButton {
                id: leftButton

                checkable: true
                iconSize.width: 24;
                iconSize.height: 24;
                fixedWidth: 52
                width: fixedWidth
                fixedHeight: 28
                height: fixedHeight
                styleSheetFile: "styledbuttonleft.css";

                iconFromFile: blueHigh ? "images/alignmentleft-h-icon.png" : "images/alignmentleft-icon.png"

                checked: backendValues.horizontalAlignment.value == "AlignLeft"

                onClicked: {
                    backendValues.horizontalAlignment.value = "AlignLeft";
                    checked = true;
                    centerButton.checked =  false;
                    rightButton.checked = false;
                }

                ExtendedFunctionButton {
                    backendValue:   backendValues.horizontalAlignment;
                    y: 7
                    x: 2
                }

            }
            QPushButton {
                id: centerButton
                x: 52
                checkable: true
                iconSize.width: 24;
                iconSize.height: 24;
                fixedWidth: 31
                width: fixedWidth
                fixedHeight: 28
                height: fixedHeight
                styleSheetFile: "styledbuttonmiddle.css";

                iconFromFile: blueHigh ? "images/alignmentcenterh-h-icon.png" : "images/alignmentcenterh-icon.png"

                checked: backendValues.horizontalAlignment.value == "AlignHCenter"

                onClicked: {
                    backendValues.horizontalAlignment.value = "AlignHCenter";
                    checked = true;
                    leftButton.checked =  false;
                    rightButton.checked = false;
                }

            }
            QPushButton {
                id: rightButton
                x: 83
                checkable: true
                iconSize.width: 24;
                iconSize.height: 24;
                fixedWidth: 31
                width: fixedWidth
                fixedHeight: 28
                height: fixedHeight
                styleSheetFile: "styledbuttonright.css";

                iconFromFile: blueHigh ? "images/alignmentright-h-icon.png" : "images/alignmentright-icon.png"

                checked: backendValues.horizontalAlignment.value == "AlignRight"

                onClicked: {
                    backendValues.horizontalAlignment.value = "AlignRight";
                    checked = true;
                    centerButton.checked =  false;
                    leftButton.checked = false;
                }
            }
        }
    }
}
