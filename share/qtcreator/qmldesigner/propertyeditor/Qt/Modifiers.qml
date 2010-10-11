import Qt 4.7
import Bauhaus 1.0

GroupBox {
    finished: finishedNotify;
    caption: qsTr("Manipulation")
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
                    property variant backendValueValue: backendValues.opacity.value;
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
        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: "Origin";
                }
                ComboBox {
                    minimumWidth: 20
                    baseStateFlag: isBaseState

                    items : { [
                            "TopLeft", "Top", "TopRight", "Left", "Center", "Right", "BottomLeft", "Bottom",
                            "BottomRight"
                            ] }
										
					backendValue: backendValues.transformOrigin
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
                    property variant backendValueValue: backendValues.scale.value;
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
            caption: qsTr("Rotation")
            baseStateFlag: isBaseState;
            step: 10;
            minimumValue: 0;
            maximumValue: 360;
        }

        IntEditor {
            backendValue: backendValues.z == undefined ? 0 : backendValues.z
            caption: qsTr("z")
            baseStateFlag: isBaseState;
            step: 1;
            minimumValue: -100;
            maximumValue: 100;
        }
    }
}
