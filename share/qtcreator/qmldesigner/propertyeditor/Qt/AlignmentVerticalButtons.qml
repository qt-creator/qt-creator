import Qt 4.7
import Bauhaus 1.0

QGroupBox {
    id: alignmentVerticalButtons

    property variant theValue: backendValues.verticalAlignment.value;

    property bool blueHigh: false

    property bool baseStateFlag: isBaseState;

    property variant backendValue: backendValues.verticalAlignment;


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
            topButton.checked = theValue == "AlignTop";
            centerButton.checked = theValue == "AlignVCenter";
            bottomButton.checked = theValue == "AlignBottom";
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
                id: topButton;
                checkable: true
                iconSize.width: 24;
                iconSize.height: 24;
                fixedWidth: 41
                width: fixedWidth
                fixedHeight: 28
                height: fixedHeight
                styleSheetFile: "styledbuttonleft.css";

                iconFromFile: blueHigh ? "images/alignmenttop-h-icon.png" : "images/alignmenttop-icon.png"

                checked: backendValues.verticalAlignment.value == "AlignTop"

                onClicked: {
                    backendValues.verticalAlignment.value = "AlignTop";
                    checked = true;
                    bottomButton.checked = false;
                    centerButton.checked = false;
                }

                ExtendedFunctionButton {
                    backendValue:   backendValues.verticalAlignment;
                    y: 7
                    x: 2
                }

            }
            QPushButton {
                x: 41
                id: centerButton;
                checkable: true
                iconSize.width: 24;
                iconSize.height: 24;
                fixedWidth: 31
                width: fixedWidth
                fixedHeight: 28
                height: fixedHeight
                styleSheetFile: "styledbuttonmiddle.css";

                iconFromFile: blueHigh ? "images/alignmentmiddle-h-icon.png" : "images/alignmentmiddle-icon.png"

                checked: backendValues.verticalAlignment.value == "AlignVCenter"

                onClicked: {
                    backendValues.verticalAlignment.value = "AlignVCenter";
                    checked = true;
                    bottomButton.checked = false;
                    topButton.checked = false;
                }

            }
            QPushButton {
                x: 72
                id: bottomButton;
                checkable: true
                iconSize.width: 24;
                iconSize.height: 24;
                fixedWidth: 31
                width: fixedWidth
                fixedHeight: 28
                height: fixedHeight
                styleSheetFile: "styledbuttonright.css";

                iconFromFile: blueHigh ? "images/alignmentbottom-h-icon.png" : "images/alignmentbottom-icon.png"

                checked: backendValues.verticalAlignment.value == "AlignBottom"

                onClicked: {
                    backendValues.verticalAlignment.value = "AlignBottom";
                    checked = true;
                    centerButton.checked = false;
                    topButton.checked = false;
                }

            }
        }

    }

}
