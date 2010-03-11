import Qt 4.6
import Bauhaus 1.0

QExtGroupBox {
    id: colorGroupBox

    property var finished;
    property var backendColor
    property var oldMaximumHeight;

    property var startupCollapse: selectionChanged;
    property var firstTime: true;
	property var caption: ""

    onFinishedChanged: {
        oldMaximumHeight = maximumHeight;
        //visible = false;
        //visible = true;
        //if (finished)
        //collapsed = true;
    }

    onStartupCollapseChanged: {
        oldMaximumHeight = maximumHeight;
        if (!collapsed && firstTime) {
            collapsed = true;
            colorButton.checked = false;
            firstTime = false;
        }
    }


    property var baseStateFlag: isBaseState
    onBaseStateFlagChanged: {
        evaluate();
    }
    onBackendColorChanged: {
        evaluate();
    }
    property var isEnabled: colorGroupBox.enabled
    onIsEnabledChanged: {
        evaluate();
    }
    property bool isInModel: (backendColor === undefined || backendColor === null) ? false: backendColor.isInModel;
    onIsInModelChanged: {
        evaluate();
    }
    property bool isInSubState: (backendColor === undefined || backendColor === null) ? false: backendColor.isInSubState;
    onIsInSubStateChanged: {
        evaluate();
    }

    Script {
        function evaluate() {
            if (!enabled) {
                valueSpinBox.setStyleSheet("color: "+scheme.disabledColor);
                hueSpinBox.setStyleSheet("color: "+scheme.disabledColor);
                saturationSpinBox.setStyleSheet("color: "+scheme.disabledColor);
            } else {
                if (baseStateFlag) {
                    if (backendColor != null && backendColor.isInModel) {
                        valueSpinBox.setStyleSheet("color: "+scheme.changedBaseColor);
                        hueSpinBox.setStyleSheet("color: "+scheme.changedBaseColor);
                        saturationSpinBox.setStyleSheet("color: "+scheme.changedBaseColor);
                    } else {
                        valueSpinBox.setStyleSheet("color: "+scheme.defaultColor);
                        hueSpinBox.setStyleSheet("color: "+scheme.defaultColor);
                        saturationSpinBox.setStyleSheet("color: "+scheme.defaultColor);
                    }
                } else {
                    if (backendColor != null && backendColor.isInSubState) {
                        valueSpinBox.setStyleSheet("color: "+scheme.changedStateColor);
                        hueSpinBox.setStyleSheet("color: "+scheme.changedStateColor);
                        saturationSpinBox.setStyleSheet("color: "+scheme.changedStateColor);
                    } else {
                        valueSpinBox.setStyleSheet("color: "+scheme.defaultColor);
                        hueSpinBox.setStyleSheet("color: "+scheme.defaultColor);
                        saturationSpinBox.setStyleSheet("color: "+scheme.defaultColor);
                    }
                }
            }
        }
    }

    ColorScheme { id:scheme; }

    QWidget {
        id: colorButtonWidget
        height: 32
        width: colorGroupBox.width
        layout: HorizontalLayout {
            topMargin: 4
            rightMargin: 10;

            Label {
                text: colorGroupBox.caption
            }

            LineEdit {
                backendValue: colorGroupBox.backendColor
                baseStateFlag: isBaseState
            }

            ColorButton {
                id: colorButton
                color: colorGroupBox.backendColor.value;
                checkable: true;
                checked: false;
                minimumHeight: 18;
                minimumWidth: 18;

                onClicked: {
				    if (colorGroupBox.animated)
					    return;
                    if (checked) {
                        colorGroupBox.collapsed = false;
                        colorButtonWidget.visible = true;
                        } else {
                        colorGroupBox.collapsed = true;
                        colorButtonWidget.visible = true;
                        }
                }

            }
            QWidget {

            }

        }
    }



    layout: VerticalLayout {
        topMargin: 36

        QWidget {
            layout: HorizontalLayout {

                ColorBox {
                    id: colorControl;
                    property var backendColor: colorGroupBox.backendColor.value;
                    color: colorGroupBox.backendColor.value;
                    onColorChanged: if (colorGroupBox.backendColor.value != color) {
                        colorGroupBox.backendColor.value = color;
                    }
                }

                HueControl {
                    id: hueControl;
                    hue: colorControl.hue;
                    onHueChanged: if (colorControl.hue != hue) colorControl.hue=hue;
                }

                QWidget {
                    maximumWidth: 100
                    layout: VerticalLayout {
                        topMargin: 4
                        bottomMargin: 4
                        rightMargin: 4
                        spacing: 2
                        QWidget {
                            layout: HorizontalLayout {
                                Label {
                                    text: "H"
                                    fixedWidth: 15
                                }

                                QSpinBox {
                                    id: hueSpinBox
                                    maximum: 359
                                    value: colorControl.hue;
                                    onValueChanged: if (colorControl.hue != value)
                                        colorControl.hue=value;
                                }

                            }

                        }
                        QWidget {
                            layout: HorizontalLayout {
                                Label {
                                    text: "S"
                                    fixedWidth: 15
                                }
                                QSpinBox {
                                    id: saturationSpinBox
                                    maximum: 255
                                    value: colorControl.saturation;
                                    onValueChanged: if (colorControl.saturation !=value)
                                    colorControl.saturation=value;
                                }
                            }

                        }
                        QWidget {
                            layout: HorizontalLayout {
                                Label {
                                    text: "B"
                                    fixedWidth: 15
                                }
                                QSpinBox {
                                    id: valueSpinBox
                                    maximum: 255
                                    value: colorControl.value;
                                    onValueChanged: if (Math.floor(colorControl.value)!=value)
                                    colorControl.value=value;
                                }
                            }
                        }

                    }
                }
            }
        }
    }
}
