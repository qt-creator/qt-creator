import Qt 4.7
import Bauhaus 1.0

GroupBox {
    id: fontGroupBox
    caption: qsTr("Font")
    property variant showStyle: false
    layout: VerticalLayout {

        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: qsTr("Font")
                }

                FontComboBox {
                    backendValue: backendValues.font_family
                    baseStateFlag: isBaseState
                }
            }
        }        
        QWidget {
            id: sizeWidget
            property bool selectionFlag: selectionChanged
            
            property bool pixelSize: sizeType.currentText == "pixels"
            property bool isSetup;
            
            onSelectionFlagChanged: {
                isSetup = true;
                sizeType.currentText = "points";
                if (backendValues.font_pixelSize.isInModel)
                    sizeType.currentText = "pixels";
                isSetup = false;
            }            
            layout: HorizontalLayout {
                Label {
                    text: qsTr("Size")
                }
                SpinBox {
                    visible: !sizeWidget.pixelSize
                    backendValue: backendValues.font_pointSize
                    baseStateFlag: isBaseState;
                }                
                SpinBox {
                    visible: sizeWidget.pixelSize
                    backendValue: backendValues.font_pixelSize
                    baseStateFlag: isBaseState;
                }                
                QComboBox {
                    id: sizeType
                    maximumWidth: 60
                    items : { ["pixels", "points"] }
                    onCurrentTextChanged: {
                        if (sizeWidget.isSetup)
                            return;
                        if (currentText == "pixels") {
                            backendValues.font_pointSize.resetValue();
                            backendValues.font_pixelSize.value = 8;
                        } else {
                            backendValues.font_pixelSize.resetValue();
                            }
                    }
                }
            }
        }               
        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: qsTr("Font Style")
                }
                FontStyleButtons {}
            }
        }
        QWidget {
            visible: showStyle
            layout: HorizontalLayout {                
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
