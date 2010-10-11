import Qt 4.7
import Bauhaus 1.0

QGroupBox {
    id: fontStyleButtons

    property int buttonWidth: 46
    layout: HorizontalLayout {

        QWidget {
            fixedHeight: 32

            FlagedButton {
                checkable: true
                iconSize.width: 24;
                iconSize.height: 24;
                fixedWidth: buttonWidth
                width: fixedWidth
                fixedHeight: 28
                height: fixedHeight
                styleSheetFile: "styledbuttonleft.css";
                checked: backendValues.font_bold.value;
                backendValue: backendValues.font_bold;

                iconFromFile: flagActive ? "images/bold-h-icon.png" : "images/bold-icon.png"

                onClicked: {
                    backendValues.font_bold.value = checked;
                }

                ExtendedFunctionButton {
                    backendValue:   backendValues.font_bold;
                    y: 7
                    x: 2
                }
            }
            FlagedButton {
                x: buttonWidth
                checkable: true
                fixedWidth: buttonWidth
                width: fixedWidth
                fixedHeight: 28
                iconSize.width: 24;
                iconSize.height: 24;
                height: fixedHeight
                iconFromFile: flagActive ? "images/italic-h-icon.png" : "images/italic-icon.png"

                styleSheetFile: "styledbuttonmiddle.css";
                checked: backendValues.font_italic.value;
                backendValue: backendValues.font_italic;

                onClicked: {
                    backendValues.font_italic.value = checked;
                }

                ExtendedFunctionButton {
                    backendValue:   backendValues.font_italic
                    y: 7
                    x: 2
                }
            }
            FlagedButton {
                x: buttonWidth * 2
                checkable: true
                fixedWidth: buttonWidth
                width: fixedWidth
                fixedHeight: 28
                iconSize.width: 24;
                iconSize.height: 24;
                height: fixedHeight
                iconFromFile:  flagActive ? "images/underline-h-icon.png" : "images/underline-icon.png"

                styleSheetFile: "styledbuttonmiddle.css";
                checked: backendValues.font_underline.value;
                backendValue: backendValues.font_underline;

                onClicked: {
                    backendValues.font_underline.value = checked;
                }

                ExtendedFunctionButton {
                    backendValue:   backendValues.font_underline;
                    y: 7
                    x: 2
                }
            }
            FlagedButton {
                x: buttonWidth * 3
                checkable: true
                fixedWidth: buttonWidth
                width: fixedWidth
                fixedHeight: 28
                iconSize.width: 24;
                iconSize.height: 24;
                height: fixedHeight
                iconFromFile: flagActive ? "images/strikeout-h-icon.png" : "images/strikeout-icon.png"

                styleSheetFile: "styledbuttonright.css";
                checked: backendValues.font_strikeout.value;
                backendValue: backendValues.font_strikeout;

                onClicked: {
                    backendValues.font_strikeout.value = checked;
                }

                ExtendedFunctionButton {
                    backendValue:   backendValues.font_strikeout;
                    y: 7
                    x: 2
                }
            }
        }
    }
}
