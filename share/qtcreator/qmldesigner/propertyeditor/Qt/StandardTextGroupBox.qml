import Qt 4.6
import Bauhaus 1.0

GroupBox {
    id: StandardTextGroupBox

    caption: "Text";

    property bool showStyleColor: false;

    layout: VerticalLayout {
        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: "Text"
                }

                LineEdit {
                    backendValue: backendValues.text
                }

                CheckBox {
                    fixedWidth: 70
                    baseStateFlag: isBaseState;
                    text: "Wraps";
                    checkable: true;
                    backendValue: backendValues.wrap;
                }
            }
        }



        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: "Alignment"
                }

                ComboBox {
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

                ComboBox {
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
        ColorWidget {
            text: "Color:";
            color: backendValues.color.value;
            onColorChanged: {
                backendValues.color.value = strColor;
            }
        }
        ColorWidget {
            visible: showStyleColor
            text: "Style color:";
            color: (backendValues.styleColor === undefined || backendValues.styleColor === null)
        ? "#000000" : backendValues.styleColor.value;
        onColorChanged: {
                backendValues.styleColor.value = strColor;
            }
        }
    }
}
