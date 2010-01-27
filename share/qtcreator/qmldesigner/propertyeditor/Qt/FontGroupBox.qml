import Qt 4.6
import Bauhaus 1.0

GroupBox {
    id: FontGroupBox
    caption: "Font";

    layout: VerticalLayout {

        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: "Font"
                }
                QFontComboBox {
                    minimumWidth: 200
                }
                IntEditor {
                    caption: "Size"
                    slider: false
                    backendValue: backendValues.font_pointSize
                    baseStateFlag: isBaseState;
                }

            }
        }

        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: "Style"
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
    }
}
