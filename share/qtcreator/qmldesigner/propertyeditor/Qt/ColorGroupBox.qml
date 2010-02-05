import Qt 4.6
import Bauhaus 1.0

QExtGroupBox {
    id: colorGroupBox

    property var finished;

    property var backendColor

    property var oldMaximumHeight;
    height: 180

    onFinishedChanged: {
        maximumHeight = height;
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
            maximumHeight: 140;
            layout: HorizontalLayout {

                ColorBox {
                    color: colorGroupBox.backendColor.value;
                    onColorChanged: {
                        colorGroupBox.backendColor.value = color;
                    }
                    hue: hueControl.hue;
                }

                HueControl {
                    id: hueControl;
                    color: colorGroupBox.backendColor.value;
                    onColorChanged: {
                        colorGroupBox.backendColor.value = color;
                    }
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

                                }

                            }
                        }
						
						QWidget {
						}
                    }
                }
            }
        }
    }
}
