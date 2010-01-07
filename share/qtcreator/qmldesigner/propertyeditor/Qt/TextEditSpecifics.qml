import Qt 4.6
import Bauhaus 1.0

GroupBox {
    maximumHeight: 600;

    finished: finishedNotify;
    caption: "Text Edit";

    layout: QVBoxLayout {
        id: textSpecifics;
        topMargin: 20;
        bottomMargin: 2;
        leftMargin: 4;
        rightMargin: 4;
        QWidget {
            layout: QHBoxLayout {
                topMargin: 8;
                bottomMargin: 2;
                leftMargin: 10;
                rightMargin: 0;
                spacing: 20;
                QLabel {
                    alignment: "AlignTop";
                    text: "Text:"
                    font.bold: true;
                }
                QTextEdit {
                    minimumHeight: 80;
                    property var localText: backendValues.text.value;
                    onLocalTextChanged: {
                        if (localText != plainText)
                            plainText = localText;
                    }

                    onTextChanged: {
                        backendValues.text.value = plainText;
                    }

                }
            }
        }
        QWidget {
            layout: QHBoxLayout {
                topMargin: 2;
                bottomMargin: 2;
                leftMargin: 10;
                rightMargin: 10;
                QLabel {
                    text: "wrap:"
                    font.bold: true;
                }
                CheckBox {
                    text: "isWrapping";
					baseStateFlag: isBaseState;
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
                bottomMargin: 8;
                leftMargin: 6;
                rightMargin: 6;
                QLabel {
                    text: "Text Format:"
                    font.bold: true;
                }

                QComboBox {
                    minimumHeight: 22;
                    items : { ["PlainText", "RichText", "AutoText"] }
                    currentText: backendValues.textFormat.value;
                    onItemsChanged: {
                        currentText =  backendValues.textFormat.value;
                    }

                    onCurrentTextChanged: {
                        if (count == 3)
                        backendValues.textFormat.value = currentText;
                    }
                }
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

        QWidget {
            layout: QHBoxLayout {
                topMargin: 4;
                bottomMargin: 2;
                leftMargin: 6;
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
                    backendValue:  backendValues. focusOnPress.value;
                }

                CheckBox {
                    text: "Persistent Selection";
					baseStateFlag: isBaseState;
                    checkable: true;
                    backendValue: backendValues.persistentSelection.value;
                }

            }
        }

    }
}
