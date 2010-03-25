import Qt 4.6
import Bauhaus 1.0

GroupBox {
    id: fontGroupBox
    caption: "Font";
    property var showStyle: false

    layout: VerticalLayout {

        QWidget {
            layout: HorizontalLayout {
			    rightMargin: 12
                Label {
                    text: qsTr("Font")
                }
				
                FontComboBox {
                    backendValue: backendValues.font_family
                    baseStateFlag: isBaseState
                }
            }
        }

        IntEditor {
            maximumWidth: 200
            caption: qsTr("Size")
            slider: false
            backendValue: backendValues.font_pointSize
            baseStateFlag: isBaseState;
        }

        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: qsTr("Font Style")
                }

                CheckBox {
                    text: qsTr("Bold")
                    backendValue: backendValues.font_bold
                    baseStateFlag: isBaseState;
                    checkable: true;
                }
                CheckBox {
                    text: qsTr("Italic")
                    backendValue: backendValues.font_italic
                    baseStateFlag: isBaseState;
                    checkable: true;
                }

            }
        }

        QWidget {
            visible: showStyle
            layout: HorizontalLayout {
			    rightMargin: 12
                Label {
                    text: qsTr("Style")
                }

                ComboBox {
                    baseStateFlag:isBaseState
                    backendValue: (backendValues.style === undefined) ? dummyBackendValue : backendValues.style
                    items : { ["Normal", "Outline", "Raised", "Sunken"] }
                    currentText: backendValue.value;
                    onItemsChanged: {
                        currentText =  backendValue.value;
                    }
                }
            }
        }
    }
}
