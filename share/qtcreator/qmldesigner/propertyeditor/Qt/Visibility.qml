import Qt 4.6
import Bauhaus 1.0

GroupBox {
    finished: finishedNotify;
    caption: "Visiblity"
    maximumHeight: 200;
    id: visibility;

    layout: VerticalLayout {

        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: "Visibility"
                }

                CheckBox {
                    id: visibleCheckBox;
                    text: "Is visible";
                    backendValue: backendValues.visible === undefined ? false : backendValues.visible;
                    baseStateFlag: isBaseState;
                    checkable: true;
                }
                CheckBox {
                    id: clipCheckBox;
                    text: "Clip Content";
                    backendValue: backendValues.clip === undefined ? false : backendValues.clip;
                    baseStateFlag: isBaseState;
                    checkable: true;
                }
            }
        }
        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: "Opacity"
                }

                DoubleSpinBox {
                    text: ""
                    id: opacitySpinBox;
                    backendValue: backendValues.opacity === undefined ? null : backendValues.opacity
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
                    backendValue: backendValues.opacity === undefined ? null : backendValues.opacity
                    onValueChanged: {
                        if (backendValues.opacity !== undefined)
                        backendValues.opacity.value = value / 100;
                    }
                }
            }
        }       
    }
}
