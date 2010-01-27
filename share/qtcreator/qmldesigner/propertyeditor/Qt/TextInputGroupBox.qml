import Qt 4.6
import Bauhaus 1.0

GroupBox {

    id: TextInputGroupBox
    caption: "Text Input";

    layout: VerticalLayout {

        ColorWidget {
            text: "Selection Color:";
            color: backendValues.selectionColor.value;
            onColorChanged: {
                backendValues.selectionColor.value = strColor;
            }
        }

        ColorWidget {
            text: "Selected Text Color:";
            color: backendValues.selectedTextColor.value;
            onColorChanged: {
                backendValues.selectedTextColor.value = strColor;
            }
        }



        QWidget {
            layout: HorizontalLayout {

                CheckBox {

                    text: "Read Only";
                    baseStateFlag: isBaseState;
                    checkable: true;
                    backendValue: backendValues.readOnly;
                }

                CheckBox {

                    text: "Cursor Visible";
                    baseStateFlag: isBaseState;
                    checkable: true;
                    backendValue: backendValues.cursorVisible;

                }

                CheckBox {
                    text: "Focus On Press";
                    baseStateFlag: isBaseState;
                    checkable: true;
                    backendValue:  backendValues. focusOnPress;
                }
            }
        }
    }
}
