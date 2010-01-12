import Qt 4.6
import Bauhaus 1.0

GroupBox {
    maximumHeight: 200;

    finished: finishedNotify;
    caption: "Text";

    layout: QVBoxLayout {
        id: textSpecifics;
        topMargin: 4;
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
                QCheckBox {

                    text: "isWrapping";
                    checkable: true;
                    checked: backendValues.wrap.value;
                    onToggled: {
                        backendValues.wrap.value = checked;
                    }
                }
            }
        }

        FontWidget {
            text: "Font:";
            dataFont: backendValues.font.value;
            onDataFontChanged: {
                backendValues.font.value = dataFont;
            }
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
                    text: "Alignment:"
                    font.bold: true;
                }
                QLabel {
                    styleSheet: "QLabel {font: normal }";
                    text: backendValues.horizontalAlignment.value;
                }

                QComboBox {
                    items : { ["AlignLeft", "AlignRight", "AlignHCenter"] }
                    currentText: backendValues.hAlign.value;
                    onItemsChanged: {
                        currentText =  backendValues.horizontalAlignment.value;
                        print(currentText);
                    }

                    onCurrentTextChanged: {
                        if (count == 3)
                        print("set");
                        print(currentText);
                        backendValues.horizontalAlignment.value = currentText;
                        print(currentText);
                        print("set");
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
