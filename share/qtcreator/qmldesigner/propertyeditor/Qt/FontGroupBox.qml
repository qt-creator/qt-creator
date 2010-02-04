import Qt 4.6
import Bauhaus 1.0

GroupBox {
    id: fontGroupBox
    caption: "Font";
    property var showStyle: false

    layout: VerticalLayout {

        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: "Font"
                }                
                QFontComboBox {
                }
            }
        }

        IntEditor {
            maximumWidth: 200
            caption: "Size"
            slider: false
            backendValue: backendValues.font_pointSize
            baseStateFlag: isBaseState;
        }

        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: "Font Style"
                }

                CheckBox {
                    text: "Bold";
                    backendValue: backendValues.font_bold
                    baseStateFlag: isBaseState;
                    checkable: true;
                }
                CheckBox {
                    text: "Italic";
                    backendValue: backendValues.font_italic
                    baseStateFlag: isBaseState;
                    checkable: true;
                }

            }
        }

        QWidget {
            visible: showStyle
            layout: HorizontalLayout {
                Label {
                    text: "Style"
                }

                ComboBox {
                    items : { ["Normal", "Outline", "Raised", "Sunken"] }
                    currentText: backendValues.style.value;
                    onItemsChanged: {
                        currentText =  backendValues.style.value;
                    }
                    onCurrentTextChanged: {
                        if (count == 4)
                        backendValues.style.value = currentText;
                    }
                }
            }
        }
    }
}
