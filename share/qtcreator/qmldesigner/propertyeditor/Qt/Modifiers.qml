import Qt 4.6

GroupBox {



    finished: finishedNotify;
    caption: "Modifiers"
    minimumHeight: 30;
    minimumHeight: 180;
    id: Mofifiers;

    layout: QVBoxLayout {
        topMargin: 2;
        bottomMargin: 6;
        leftMargin: 0;
        rightMargin: 0;

        QWidget {
            id: contentWidget;
            maximumHeight: 240;

            layout: QHBoxLayout {
                topMargin: 0;
                bottomMargin: 0;
                leftMargin: 10;
                rightMargin: 10;
                QWidget {
                    layout: QVBoxLayout {
                        topMargin: 0;
                        bottomMargin: 30;
                        leftMargin: 0;
                        rightMargin: 0;

                        QLabel {
                            minimumHeight: 50;
                            text: "Rotation:"
                            font.bold: true;
                        }

                        QLabel {
                            text: "Scale:"
                            font.bold: true;
                        }

                        QLabel {
                            text: "Scale:"
                            text: ""
                            font.bold: true;
                        }

                        QLabel {
                            text: "Opacity:"
                            font.bold: true;
                        }
                        QLabel {
                            text: "Visibility:"
                            font.bold: true;
                        }
                    }
                }
                QWidget {
                    layout: QVBoxLayout {

                        topMargin: 02;
                        bottomMargin: 2;
                        leftMargin: 5;
                        rightMargin: 0;

                        QWidget {
                            layout: QHBoxLayout {
                                topMargin: 3;
                                bottomMargin: 0;
                                leftMargin: 0;
                                rightMargin: 0;
                                SpinBox {
                                    id: RotationSpinBox;
                                    backendValue: backendValues.rotation;
                                    onBackendValueChanged: {
                                        if (backendValue.value > 180)
                                            RotationDial.value = (backendValue.value - 360);
                                        else
                                            RotationDial.value = backendValue.value;
                                    }
                                    minimum: 0;
                                    maximum: 360;
                                    baseStateFlag: isBaseState;
                                }
                                QDial {
                                    id: RotationDial;
                                    wrapping: true;
                                    focusPolicy: "Qt::ClickFocus";
                                    minimumHeight: 20;
                                    maximumHeight: 50;
                                    minimum: -180;
                                    maximum: 180;
                                    singleStep: 45;
                                    onValueChanged : {
                                        if (value < 0)
                                            RotationSpinBox.backendValue.value = 360 + value;
                                        else
                                            RotationSpinBox.backendValue.value = value;
                                    }
                                }
                            }
                        }                                                                        
                        QWidget {
                            layout: QHBoxLayout {
                                topMargin: 10;
                                bottomMargin: 0;
                                leftMargin: 0;
                                rightMargin: 0;
                                DoubleSpinBox {
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
                                QSlider {
                                    id: ScaleSlider;
                                    orientation: "Qt::Horizontal";
                                    minimum: 1;
                                    maximum: 100;
                                    singleStep: 1;
                                    onValueChanged: {
                                        backendValues.scale.value = value / 10;
                                    }
                                }
                            }
                        }

                        QWidget {
                            layout: QHBoxLayout {
                                topMargin: 5;
                                bottomMargin: 5;
                                leftMargin: 10;
                                rightMargin: 0;

                                QLabel {
                                    text: "Origin: ";
                                }
                                QComboBox {
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
                            layout: QHBoxLayout {
                                topMargin: 5;
                                bottomMargin: 10;
                                leftMargin: 0;
                                rightMargin: 0;

                                DoubleSpinBox {
                                    id: OpacitySpinBox;
                                    backendValue: backendValues.opacity === undefined ? null : backendValues.opacity
                                    minimumWidth: 60;
                                    minimum: 0;
                                    maximum: 1;
                                    singleStep: 0.1
                                    baseStateFlag: isBaseState;
                                }
                                QSlider {
                                    orientation: "Qt::Horizontal";
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
                        CheckBox {
                            id: VisibleCheckBox;
                            text: "item visibilty";
                            backendValue: backendValues.visible === undefined ? false : backendValues.visible;
                            baseStateFlag: isBaseState;
                            checkable: true;
                        }
                        CheckBox {
                            id: ClipCheckBox;
                            text: "clipping item";
                            backendValue: backendValues.clip === undefined ? false : backendValues.clip;
                            baseStateFlag: isBaseState;
                            checkable: true;
                        }
                    }
                }
            }
        }
    }
}
