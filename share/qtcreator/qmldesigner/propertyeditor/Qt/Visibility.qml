import Qt 4.6
import Bauhaus 1.0

GroupBox {
    finished: finishedNotify;
    caption: "Visibility"
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
                    property var backendValueValue: backendValues.opacity.value;
                    minimumWidth: 60;
                    minimum: 0;
                    maximum: 1;
                    singleStep: 0.1
                    baseStateFlag: isBaseState;
                    onBackendValueValueChanged: {
                        opacitySlider.value = backendValue.value * 100;
                    }
                }
                SliderWidget {
                    id: opacitySlider
                    minimum: 0
                    maximum: 100
                    singleStep: 5;
                    backendValue: backendValues.opacity
                    onValueChanged: {
                        backendValues.opacity.value = value / 100;
                    }
                }
            }
        }       
    }
}
