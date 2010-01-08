import Qt 4.6
import Bauhaus 1.0

QWidget { //This is a special DoubleSpinBox that does color coding for states
    id: DoubleSpinBox;

    property var backendValue;
    property var baseStateFlag;
    property alias singleStep: box.singleStep
    property alias minimum: box.minimum
    property alias maximum: box.maximum

    minimumHeight: 22;

    onBaseStateFlagChanged: {
        evaluate();
        setIcon();
    }

    onBackendValueChanged: {
        evaluate();
        setIcon();
    }

    Script {
        function evaluate() {
            if (baseStateFlag) {
                if (backendValue != null && backendValue.isInModel)
                DoubleSpinBox.setStyleSheet("color: white;");
                else
                DoubleSpinBox.setStyleSheet("color: gray;");
                } else {
                if (backendValue != null && backendValue.isInSubState)
                DoubleSpinBox.setStyleSheet("color: #7799FF;");
                else
                DoubleSpinBox.setStyleSheet("color: gray;");
                }
        }
    }

    layout: QHBoxLayout {
        topMargin: 0;
        bottomMargin: 0;
        leftMargin: 0;
        rightMargin: 2;
        spacing: 0;

        QDoubleSpinBox {
            id: box;
            decimals: 2;
            keyboardTracking: false;
            enabled: (backendValue == null || backendValue.complexNode == null) ? false : !backendValue.isBound && !backendValue.complexNode.exists
            value: DoubleSpinBox.backendValue == null ? .0 : DoubleSpinBox.backendValue.value;
            onValueChanged: {
                if (DoubleSpinBox.backendValue != null)
                    DoubleSpinBox.backendValue.value = value;
            }
            onFocusChanged: {
                //extendedSwitches.active = focus;
                //extendedSwitches.backendValue = backendValue;
            }

            onMouseOverChanged: {
                    //extendedButton.active = mouseOver;
            }
        }
    }

    QToolButton {
        property bool active: false;
        id: extendedButton;
        iconFromFile: "images/expression.png";
        visible: false;
        width: 14;
        height: 14;
        y: box.y + 2;
        x: box.x + 2;
        focusPolicy: "Qt::NoFocus";
        opacity: 0;

        opacity: Behavior {
            NumberAnimation {
                    easing: "easeInSine"
                    duration: 200
                }
            }

        onPressed: {

        }

        Script {
            function setIcon() {
                if (backendValue == null)
                    extendedButton.iconFromFile = "images/placeholder.png"
                else if (backendValue.isBound) {
                    extendedButton.iconFromFile = "images/expression.png"
                } else {
                    if (backendValue.complexNode != null && backendValue.complexNode.exists) {
                        extendedButton.iconFromFile = "images/behaivour.png"
                    } else {
                        extendedButton.iconFromFile = "images/placeholder.png"
                    }
                }
            }
    }

        onMouseOverChanged: {
            if (mouseOver) {
                iconFromFile = "images/submenu.png";
                opacity = 1;
            } else {
                setIcon();
                opacity = 0;
            }
        }

        onActiveChanged: {
            if (active) {
                setIcon();
                opacity = 1;
            } else {
                opacity = 0;
            }
        }

        menu: QMenu {
            actions: [
                QAction {
                    text: "Reset";
                },
                QAction {
                    text: "Set Expression";
                },
                QAction {
                    text: "Add Behaivour";
                }
            ]
        }
        toolButtonStyle: "Qt::ToolButtonIconOnly";
        popupMode: "QToolButton::InstantPopup";
        arrowType: "Qt::NoArrow";
    }
}
