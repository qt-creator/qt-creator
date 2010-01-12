import Qt 4.6
import Bauhaus 1.0

GroupBox {

    finished: finishedNotify;
    caption: "Rect"

    minimumHeight: 340;
    maximumHeight: 340;

    layout: QVBoxLayout {
        topMargin: 12;
        bottomMargin: 20;
        leftMargin: 20;
        rightMargin: 20;

        QWidget {
            layout: QVBoxLayout {
                topMargin: 2;
                bottomMargin: 20;
                leftMargin: 0;
                rightMargin: 0;

                QWidget {
                    layout: QHBoxLayout {
                        topMargin: 8;
                        bottomMargin: 20;
                        leftMargin: 10;
                        rightMargin: 10;
                        QLabel {
                            text: "Radius:"
                            font.bold: true;
                        }
                        SpinBox {
                            id: RadiusSpinBox;
                            backendValue: backendValues.radius === undefined ? null : backendValues.radius
                            minimum: 0;
                            maximum: 100;
                            baseStateFlag: isBaseState;
                        }
                        QSlider {
                            orientation: "Qt::Horizontal";
                            minimum: 0;
                            maximum: 100;
                            singleStep: 1;
                            value: backendValues.radius === undefined ? null : backendValues.radius.value
                            onValueChanged: {
                                backendValues.radius.value = value;
                            }
                        }
                    }
                }
                QWidget {
                    layout: QHBoxLayout {
                        topMargin: 2;
                        bottomMargin: 2;
                        leftMargin: 10;
                        rightMargin: 10;
                        spacing: 20;

                        ColorWidget {
                            text: "Color:";
                            color: backendValues.color === undefined ? null : backendValues.color.value;
                            onColorChanged: {
                                backendValues.color.value = strColor;
                            }
                            //modelNode: backendValues.color.modelNode;
                            complexGradientNode: backendValues.color === undefined ? null : backendValues.color.complexNode

                            showGradientButton: true;
                        }
                        ColorWidget {
                            text: "Tint color:";
                            color: backendValues.tintColor === undefined ? "black" : backendValues.tintColor.value
                            onColorChanged: {
                                backendValues.color.value = strColor;
                            }
                        }
                    }
                }
                HorizontalLine {
                }
            }
        }
        QWidget {
            minimumHeight: 80;
            maximumHeight: 120;

            layout: QHBoxLayout {
                topMargin: 2;
                topMargin: 0;
                bottomMargin: 0;
                leftMargin: 0;
                rightMargin: 0;

                QWidget {
                    id: PenGroupBox;

                    maximumHeight: 80;

                    layout: QVBoxLayout {
                        topMargin: 10;
                        bottomMargin: 10;
                        leftMargin: 20;
                        rightMargin: 20;
                        IntEditor {

                            id: borderWidth;
                            backendValue: backendValues.border_width === undefined ? 0 : backendValues.border_width

                            caption: "Pen Width:"
                            baseStateFlag: isBaseState;

                            step: 1;
                            minimumValue: 0;
                            maximumValue: 100;
                        }
                        ColorWidget {
                            id: PenColor;
                            text: "Pen Color:";
                            minimumHeight: 20;
                            minimumWidth: 20;
                            color: backendValues.border_color.value;
                            onColorChanged: {
                                backendValues.border_color.value = strColor;
                            }
                        }
                    }
                }
            }
        }
    }
}
