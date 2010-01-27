import Qt 4.6

GroupBox {
    finished: finishedNotify;
    caption: "Manipulation"
    maximumHeight: 200;
    minimumHeight: 180;
    id: Mofifiers;

    layout: VerticalLayout {        

        QWidget {
            layout: HorizontalLayout {
                QLabel {
                    text: "Visibility"
                    alignment: "Qt::AlignRight | Qt::AlignVCenter"
                }

                CheckBox {
                    id: VisibleCheckBox;
                    text: "Is visible";
                    backendValue: backendValues.visible === undefined ? false : backendValues.visible;
                    baseStateFlag: isBaseState;
                    checkable: true;
                }
                CheckBox {
                    id: ClipCheckBox;
                    text: "Clips";
                    backendValue: backendValues.clip === undefined ? false : backendValues.clip;
                    baseStateFlag: isBaseState;
                    checkable: true;
                    fixedWidth: 100
                }
            }
        }
        QWidget {
            layout: HorizontalLayout {

                QLabel {
                    text: "Opacity"
                    alignment: "Qt::AlignRight | Qt::AlignVCenter"
                }

                DoubleSpinBox {
                    text: ""
                    id: OpacitySpinBox;
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

                QLabel {
                    text: "Origin";
                    alignment: "Qt::AlignRight | Qt::AlignVCenter"
                }
                QComboBox {
                    minimumWidth: 20
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
                QWidget {
                    fixedWidth: 100
                }

            }
        }
        QWidget {
            layout: HorizontalLayout {

                QLabel {
                    text: "Scale"
                    alignment: "Qt::AlignRight | Qt::AlignVCenter"
                }

                DoubleSpinBox {
                    text: ""
                    id: ScaleSpinBox;
                    objectName: "ScaleSpinBox";
                    backendValue: backendValues.scale;
                    minimumWidth: 60;
                    minimum: 0.01
                    maximum: 10
                    singleStep: 0.1
                    baseStateFlag: isBaseState;
                    onBackendValueChanged: {
                        ScaleSlider.value = backendValue.value * 10;
                    }
                }
                SliderWidget {
                    id: ScaleSlider;
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
            minimumValue: 0;
            maximumValue: 100;
        }
    }
}
