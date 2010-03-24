import Qt 4.6
import Bauhaus 1.0

GroupBox {

    id: textInputGroupBox
    caption: "Text Input";

    layout: VerticalLayout {       

        QWidget {
            layout: HorizontalLayout {
                Label {text: "Flags"}

                CheckBox {
                    text: "Read Only";
                    baseStateFlag: isBaseState;
                    checkable: true;
                    backendValue: backendValues.readOnly;
                }



                
            }
        }

        QWidget {
            layout: HorizontalLayout {
                Label {text: ""}

                CheckBox {

                    text: "Cursor Visible";
                    baseStateFlag: isBaseState;
                    checkable: true;
                    backendValue: backendValues.cursorVisible;

                }

            }
        }

        QWidget {
            layout: HorizontalLayout {
                Label {text: ""}
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
