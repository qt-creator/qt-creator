import Qt 4.6
import Bauhaus 1.0

GroupBox {

    id: textInputGroupBox
    caption: qsTr("Text Input")

    layout: VerticalLayout {       

        QWidget {
            layout: HorizontalLayout {
                Label {text: qsTr("Flags") }

                CheckBox {
                    text: qsTr("Read Only")
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

                    text: qsTr("Cursor Visible")
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
                    text: qsTr("Focus On Press")
                    baseStateFlag: isBaseState;
                    checkable: true;
                    backendValue:  backendValues. focusOnPress;
                }
            }
        }
    }
}
