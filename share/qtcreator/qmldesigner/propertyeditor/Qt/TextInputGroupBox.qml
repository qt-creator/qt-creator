import Qt 4.6
import Bauhaus 1.0

GroupBox {

    id: TextInputGroupBox
    caption: "Text Input";

    layout: VerticalLayout {
	
		ColorLabel {
            text: "    Selection Color"			
        }

        ColorGroupBox {

            finished: finishedNotify

            backendColor: backendValues.selectionColor
        }
		
		ColorLabel {
            text: "    Selected Text Color"			
        }

        ColorGroupBox {

            finished: finishedNotify

            backendColor: backendValues.selectedTextColor
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
