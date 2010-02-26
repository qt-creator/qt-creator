import Qt 4.6
import Bauhaus 1.0

QExtGroupBox {
    id: colorGroupBox

    property var finished;

    property var backendColor

    property var oldMaximumHeight;

    onFinishedChanged: {
        oldMaximumHeight = maximumHeight;
        visible = false;
        visible = true;
        if (finished)
        collapsed = true;
    }


    QWidget {
        id: colorButtonWidget
        height: 32
        width: colorGroupBox.width
        layout: HorizontalLayout {
            topMargin: 4
            rightMargin: 10;

            Label {
                text: "Hex. Code"
            }

            LineEdit {
                backendValue: colorGroupBox.backendColor
                baseStateFlag: isBaseState
            }

            ColorButton {
                color: colorGroupBox.backendColor.value;
                checkable: true;
                checked: false;
                minimumHeight: 18;
                minimumWidth: 18;

                onClicked: {
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
                    color: colorGroupBox.backendColor.value;
                    onColorChanged: {
                        colorGroupBox.backendColor.value = color;
                    }
                }

                HueControl {
                    id: hueControl;
                    hue: colorControl.hue;
                    onHueChanged: colorControl.hue=hue;
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
                                    maximum: 255
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
