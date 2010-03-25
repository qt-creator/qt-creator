import Qt 4.6
import Bauhaus 1.0

QToolButton {
    id: extendedFunctionButton

    property var backendValue

    function setIcon() {
        if (backendValue == null)
            extendedFunctionButton.iconFromFile = "images/placeholder.png"
        else if (backendValue.isBound) {
            extendedFunctionButton.iconFromFile = "images/expression.png"
        } else {
            if (backendValue.complexNode != null && backendValue.complexNode.exists) {
                extendedFunctionButton.iconFromFile = "images/behaivour.png"
            } else {
                extendedFunctionButton.iconFromFile = "images/placeholder.png"
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
        text: qsTr("Reset")
        onTriggered: {
            backendValue.resetValue();
        }

    },
    QAction {
        text: qsTr("Set Expression");
        onTriggered: {
            expressionEdit.globalY = extendedFunctionButton.globalY;
            expressionEdit.backendValue = extendedFunctionButton.backendValue
            expressionEdit.show();
            expressionEdit.raise();
            expressionEdit.active = true;
        }
    }
    ]
}
