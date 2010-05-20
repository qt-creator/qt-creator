import Qt 4.7
import Bauhaus 1.0

GroupBox {
    finished: finishedNotify;
    caption: qsTr("Visibility")
    maximumHeight: 200;
    id: visibility;

    layout: VerticalLayout {

        QWidget {
            layout: HorizontalLayout {
                Label {                    
                    text: qsTr("Visibility")
                }
                CheckBox {
                    id: visibleCheckBox;
                    text: qsTr("Is visible")
                    backendValue: backendValues.visible;
                    baseStateFlag: isBaseState;
                    checkable: true;
                }                
            }
        }
        QWidget {
            layout: HorizontalLayout {
                Label {                    
                    text: ""
                }
                CheckBox {
                    id: clipCheckBox;
                    text: qsTr("Clip")
                    backendValue: backendValues.clip;
                    baseStateFlag: isBaseState;
                    checkable: true;
                }
            }
        }
        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: qsTr("Opacity")
                }

                DoubleSpinBox {
                    text: ""
                    id: opacitySpinBox;
                    backendValue: backendValues.opacity
                    property variant backendValueValue: backendValues.opacity.value;
                    minimumWidth: 60;
                    minimum: 0;
                    maximum: 1;
                    singleStep: 0.1
                    baseStateFlag: isBaseState;                    
                }
                SliderWidget {
                    id: opacitySlider
                    minimum: 0
                    maximum: 100
                    property variant pureValue: backendValues.opacity.value;
                    onPureValueChanged: {
                        if (value != pureValue * 100)
                            value = pureValue * 100;
                    }
                    singleStep: 5;
                    backendValue: backendValues.opacity
                    onValueChanged: {
                    if ((value >= 0) && (value < 100))
                        backendValues.opacity.value = value / 100;
                    }
                }
            }
        }       
    }
}
