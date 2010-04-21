import Qt 4.7
import Bauhaus 1.0

GroupBox {

    id: textInputGroupBox
    caption: qsTr("Text Input")
    property bool isTextInput: false

    layout: VerticalLayout {

        QWidget {
            visible: isTextInput
            layout: HorizontalLayout {

                Label {text: "Input Mask"}

                LineEdit {
                    backendValue: backendValues.inputMask
                    baseStateFlag: isBaseState
                }
            }
        }

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
                    backendValue:  backendValues.focusOnPress;
                }
            }
        }

        QWidget {
            visible: isTextInput
            layout: HorizontalLayout {
                Label {text: ""}
                CheckBox {
                    text: qsTr("Auto Scroll")
                    baseStateFlag: isBaseState;
                    checkable: true;
                    backendValue:  backendValues.autoScroll;
                }
            }
        }
    }
}
