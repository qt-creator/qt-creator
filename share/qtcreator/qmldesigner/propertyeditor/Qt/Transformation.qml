import Qt 4.6
import Bauhaus 1.0

GroupBox {
    finished: finishedNotify;
    caption: "Transformation"
    maximumHeight: 200;
    id: transformation;

    layout: VerticalLayout {       
        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: "Origin";
                }
                ComboBox {
                    minimumWidth: 20
                    baseStateFlag: isBaseState
                    backendValue: backendValues.transformOrigin

                    items : { [
                            "TopLeft", "Top", "TopRight", "Left", "Center", "Right", "BottomLeft", "Bottom",
                            "BottomRight"
                            ] }

                            currentText: backendValues.transformOrigin.value;
                            onItemsChanged: {
                                currentText =  backendValues.transformOrigin.value;
                            }
                }
            }
        }
        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: "Scale"
                }

                DoubleSpinBox {
                    text: ""
                    id: scaleSpinBox;

                    backendValue: backendValues.scale;
                    property var backendValueValue: backendValues.scale.value;
                    minimumWidth: 60;
                    minimum: 0.01
                    maximum: 10
                    singleStep: 0.1
                    baseStateFlag: isBaseState;
                    onBackendValueValueChanged: {
                        scaleSlider.value = backendValue.value * 10;
                    }
                }
                SliderWidget {
                    id: scaleSlider;
                    backendValue: backendValues.scale;
                    minimum: 1;
                    maximum: 100;
                    singleStep: 1;
                    onValueChanged: {
                        backendValues.scale.value = value / 10;
                    }
                }
            }
        }
        IntEditor {
            backendValue: backendValues.rotation
            caption: "Rotation"
            baseStateFlag: isBaseState;
            step: 10;
            minimumValue: 0;
            maximumValue: 360;
        }

        IntEditor {
            backendValue: backendValues.z
            caption: "z"
            baseStateFlag: isBaseState;
            step: 1;
            minimumValue: -100;
            maximumValue: 100;
        }
    }
}
