import Qt 4.7
import Bauhaus 1.0

QWidget {
    property bool showGradientButton

    property bool gradient: gradientButton.checked
    property bool none: noneButton.checked
    property bool solid: solidButton.checked

    property bool setGradient: false
    property bool setNone: false
    property bool setSolid: false

    onSetGradientChanged: {
        if (setGradient == true) {
            gradientButton.checked = true;
            setGradient = false;
        }
    }

    onSetNoneChanged: {
        if (setNone == true) {
            noneButton.checked = true;
            setNone = false;
        }
    }

    onSetSolidChanged: {
        if (setSolid == true) {
            solidButton.checked = true;
            setSolid = false;
        }
    }

    fixedHeight: 28
    fixedWidth: 93
    width: fixedWidth
    height: fixedHeight
    enabled: isBaseState

    QPushButton {
        id: solidButton
        x: 0
        checkable: true
        checked: true
        fixedWidth: 31
        fixedHeight: 28
        
        
        styleSheetFile: "styledbuttonleft.css"                
        iconFromFile: "images/icon_color_solid.png"

        onToggled: {
            if (checked) {
                gradientButton.checked = false;
                noneButton.checked = false;
            }
        }
        onClicked: {
            gradientButton.checked = false;
            noneButton.checked = false;
            checked = true;
        }
    }

    QPushButton {
        visible: showGradientButton
        id: gradientButton
        x: 31
        checkable: true
        fixedWidth: 31
        fixedHeight: 28

        styleSheetFile: "styledbuttonmiddle.css"
        iconFromFile: "images/icon_color_gradient.png"

        onToggled: {
            if (checked) {
                solidButton.checked = false;
                noneButton.checked = false;
            }
        }

        onClicked: {
            solidButton.checked = false;
            noneButton.checked = false;
            checked = true;
        }
    }

    QPushButton {
        id: noneButton
        x: showGradientButton ? 62 : 31;
        checkable: true
        fixedWidth: 31
        fixedHeight: 28
        styleSheetFile: "styledbuttonright.css"
        iconFromFile: "images/icon_color_none.png"

        onToggled: {
            if (checked) {
                gradientButton.checked = false;
                solidButton.checked = false;
            }
        }

        onClicked: {
            gradientButton.checked = false;
            solidButton.checked = false;
            checked = true;
        }

    }
}
