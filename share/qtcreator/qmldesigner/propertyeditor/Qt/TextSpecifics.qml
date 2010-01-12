import Qt 4.6
import Bauhaus 1.0

GroupBox {
    maximumHeight: 280;

    finished: finishedNotify;
    caption: "Text";

    layout: QVBoxLayout {
        id: textSpecifics;
        topMargin: 12;
        bottomMargin: 2;
        leftMargin: 4;
        rightMargin: 4;
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
        QWidget {
            layout: QHBoxLayout {
                topMargin: 6;
                bottomMargin: 2;
                leftMargin: 10;
                rightMargin: 10;
                QLabel {
                    text: "wrap:"
                    font.bold: true;
                }
                CheckBox {
                    baseStateFlag: isBaseState;
                    text: "isWrapping";
                    checkable: true;
                    backendValue: backendValues.wrap;
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
                leftMargin: 10;
                rightMargin: 6;

            }
        }

        QWidget {
            layout: QHBoxLayout {
                topMargin: 4;
                bottomMargin: 2;
                leftMargin: 10;
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
                        if (count == 3);
                        backendValues.horizontalAlignment.value = currentText;
                    }

                }
            }
        }

        QWidget {
            layout: QHBoxLayout {
                topMargin: 4;
                bottomMargin: 2;
                leftMargin: 10;
                rightMargin: 6;
                QLabel {
                    text: "Vertical Alignment:"
                    font.bold: true;
                }

                QComboBox {
                    items : { ["AlignTop", "AlignBottom", "AlignVCenter"] }
                    currentText: backendValues.verticalAlignment.value;
                    onItemsChanged: {
                        currentText =  backendValues.verticalAlignment.value;
                    }

                    onCurrentTextChanged: {
                        if (count == 3)
                        backendValues.verticalAlignment.value = currentText;
                    }

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

                ColorWidget {
                    text: "Color:";
                    color: backendValues.color.value;
                    onColorChanged: {
                        backendValues.color.value = strColor;
                    }
                }
                ColorWidget {
                    text: "Style color:";
                    color: backendValues.styleColor.value;
                    onColorChanged: {
                        backendValues.styleColor.value = strColor;
                    }
                }
            }
        }
    }
}
