import Qt 4.6
import Bauhaus 1.0

QToolButton {
    id: ExtendedFunctionButton

    property var backendValue

    Script {
        function setIcon() {
            if (backendValue == null)
                ExtendedFunctionButton.iconFromFile = "images/placeholder.png"
            else if (backendValue.isBound) {
                ExtendedFunctionButton.iconFromFile = "images/expression.png"
            } else {
                if (backendValue.complexNode != null && backendValue.complexNode.exists) {
                    ExtendedFunctionButton.iconFromFile = "images/behaivour.png"
                } else {
                    ExtendedFunctionButton.iconFromFile = "images/placeholder.png"
                }
            }
        }    
    }


    onBackendValueChanged: {
        setIcon();
    }

    property bool isBoundBackend: backendValue.isBound;

    onIsBoundBackendChanged: {
        setIcon();
    }

    toolButtonStyle: "Qt::ToolButtonIconOnly"
    popupMode: "QToolButton::InstantPopup";
    property bool active: false;

    iconFromFile: "images/placeholder.png";
    width: 14;
    height: 14;
    focusPolicy: "Qt::NoFocus";

    styleSheet: "*::down-arrow, *::menu-indicator { image: none; width: 0; height: 0; }";


    onMouseOverChanged: {
        if (mouseOver) {
            iconFromFile = "images/submenu.png";
        } else {
            setIcon();
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


    actions:  [
    QAction {
        text: "Reset";
        onTriggered: {
            backendValue.resetValue();
        }

    },
    QAction {
        text: "Set Expression";
        onTriggered: {
            expressionEdit.globalY = ExtendedFunctionButton.globalY;
            expressionEdit.backendValue = ExtendedFunctionButton.backendValue
            expressionEdit.show();
            expressionEdit.raise();
            expressionEdit.active = true;
        }
    },
    QAction {
        text: "Add Behaivour";
    }
    ]
}
