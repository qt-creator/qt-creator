import Qt 4.6
import Bauhaus 1.0

GroupBox {
    finished: finishedNotify;
    caption: "Manipulation"
    maximumHeight: 200;
    minimumHeight: 180;
    id: mofifiers;

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
                    minimumWidth: 60;
                    minimum: 0;
                    maximum: 1;
                    singleStep: 0.1
                    baseStateFlag: isBaseState;
                }
                SliderWidget {
                    minimum: 0
                    maximum: 100
                    singleStep: 5;
                    value: backendValues.opacity === undefined ? 0 : (backendValues.opacity.value * 100)
                    onValueChanged: {
                        if (backendValues.opacity !== undefined)
                            backendValues.opacity.value = value / 100;
                    }
                }
            }
        }
        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: "Origin";
                }
                ComboBox {
                    minimumWidth: 20
					
					backendValue: backendValues.transformOrigin
                    items : { [
                        "TopLeft", "Top", "TopRight", "Left", "Center", "Right", "BottomLeft", "Bottom",
                        "BottomRight"
                        ] }

                    currentText: backendValues.transformOrigin.value;
                    onItemsChanged: {
                        currentText =  backendValues.transformOrigin.value;
                    }
                    onCurrentTextChanged: {
                        backendValues.transformOrigin.value = currentText;
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
            backendValue: backendValues.z == undefined ? 0 : backendValues.z
            caption: "z"
            baseStateFlag: isBaseState;
            step: 1;
            minimumValue: -100;
            maximumValue: 100;
        }
    }
}
