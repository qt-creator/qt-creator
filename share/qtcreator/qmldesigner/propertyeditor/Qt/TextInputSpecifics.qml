import Qt 4.6
import Bauhaus 1.0

GroupBox {
    maximumHeight: 420;
    minimumHeight: 420;

    finished: finishedNotify;
    caption: "Text Input";

    layout: QVBoxLayout {
        id: textSpecifics;
        topMargin: 12;
        bottomMargin: 2;
        leftMargin: 10;
        rightMargin: 10;
        QWidget {
            layout: QHBoxLayout {
                topMargin: 8;
                bottomMargin: 2;
                leftMargin: 10;
                rightMargin: 10;
                QLabel {
                    text: "Text:"
                    font.bold: true;
                }
                QLineEdit {
                    minimumHeight: 24;
                    text: backendValues.text.value;
                    onTextChanged: {
                        backendValues.text.value = text;
                    }
                }
            }
        }

        FontWidget {
            text: "Font:";

            bold: backendValues.font_bold.value;
            italic: backendValues.font_italic.value;
            family: backendValues.font_family.value;
            fontSize: backendValues.font_pointSize.value;

            onDataFontChanged: {

                if (bold)
                    backendValues.font_bold.value = bold;
                else
                    backendValues.font_bold.resetValue();

                if (italic)
                    backendValues.font_italic.value = bold;
                else
                    backendValues.font_italic.resetValue();

                backendValues.font_family.value = family;
                backendValues.font_pointSize.value = fontSize;
            }
        }

        HorizontalLine {
        }


        QWidget {
            layout: QHBoxLayout {
                topMargin: 4;
                bottomMargin: 2;
                leftMargin: 6;
                rightMargin: 6;
                QLabel {
                    text: "Horizontal Alignment:"
                    font.bold: true;
                }

                QComboBox {
                    items : { ["AlignLeft", "AlignRight", "AlignHCenter"] }
                    currentText: backendValues.horizontalAlignment.value;
                    onItemsChanged: {
                        currentText =  backendValues.horizontalAlignment.value;
                    }

                    onCurrentTextChanged: {
                        if (count == 3)
                            backendValues.horizontalAlignment.value = currentText;
                    }

                }
            }
        }

        HorizontalLine {
        }

        ColorWidget {
            minimumHeight: 20;
            maximumHeight: 20;
            text: "Color:";
            color: backendValues.color.value;
            onColorChanged: {
                backendValues.color.value = strColor;
            }
        }

        ColorWidget {
            minimumHeight: 20;
            maximumHeight: 20;
            text: "Selection Color:";
            color: backendValues.selectionColor.value;
            onColorChanged: {
                backendValues.selectionColor.value = strColor;
            }
        }

        ColorWidget {
            minimumHeight: 20;
            maximumHeight: 20;
            text: "Selected Text Color:";
            color: backendValues.selectedTextColor.value;
            onColorChanged: {
                backendValues.selectedTextColor.value = strColor;
            }
        }

        HorizontalLine {
        }


        QWidget {
            layout: QHBoxLayout {
                topMargin: 2;
                bottomMargin: 2;
                leftMargin: 10;
                rightMargin: 10;

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
            }
        }

        HorizontalLine {
        }

        QWidget {
            layout: QHBoxLayout {
                topMargin: 2;
                bottomMargin: 2;
                leftMargin: 10;
                rightMargin: 10;

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
