import Qt 4.6
import Bauhaus 1.0

GroupBox {

    id: textInputGroupBox
    caption: "Text Input";

    layout: VerticalLayout {       

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
