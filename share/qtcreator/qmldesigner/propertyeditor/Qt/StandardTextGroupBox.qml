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
            }
        }
        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: ""
                }
                CheckBox {
                    baseStateFlag: isBaseState;
                    text: "Is Wrapping";
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
		
		ColorLabel {
            text: "    Color"			
        }

        ColorGroupBox {

            finished: finishedNotify

            backendColor: backendValues.color
        }
		
		ColorLabel {
			visible: showStyleColor
            text: "    Style Color"			
        }

        ColorGroupBox {
			visible: showStyleColor
            finished: finishedNotify

            backendColor: backendValues.styleColor || null
        }
		        
    }
}
