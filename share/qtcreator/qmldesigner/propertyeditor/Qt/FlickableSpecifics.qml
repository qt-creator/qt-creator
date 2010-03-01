import Qt 4.6
import Bauhaus 1.0

QWidget {
    layout: QVBoxLayout {
        topMargin: 0
        bottomMargin: 0
        leftMargin: 0
        rightMargin: 0
        spacing: 0
        GroupBox {


            GroupBox {
                finished: finishedNotify;
                caption: "Flickable";

                layout: QVBoxLayout {
                    topMargin: 15;
                    bottomMargin: 6;
                    leftMargin: 0;
                    rightMargin: 0;

                    QWidget {
                        id: contentWidget;
                        maximumHeight: 220;

                        layout: QHBoxLayout {
                            topMargin: 0;
                            bottomMargin: 0;
                            leftMargin: 10;
                            rightMargin: 10;

                            QWidget {
                                layout: QVBoxLayout {
                                    topMargin: 0;
                                    bottomMargin: 0;
                                    leftMargin: 0;
                                    rightMargin: 0;
                                    QLabel {
                                        minimumHeight: 20;
                                        text: "Horizontal Velocity:"
                                        font.bold: true;
                                    }

                                    QLabel {
                                        minimumHeight: 20;
                                        text: "Vertical Velocity:"
                                        font.bold: true;
                                    }

                                    QLabel {
                                        minimumHeight: 20;
                                        text: "Maximum Flick Velocity:"
                                        font.bold: true;
                                    }

                                    QLabel {
                                        minimumHeight: 20;
                                        text: "Over Shoot:"
                                        font.bold: true;
                                    }
                                }
                            }

                            QWidget {
                                layout: QVBoxLayout {
                                    topMargin: 0;
                                    bottomMargin: 0;
                                    leftMargin: 0;
                                    rightMargin: 0;


                                    DoubleSpinBox {
                                        id: horizontalVelocitySpinBox;
                                        objectName: "horizontalVelocitySpinBox";
                                        backendValue: backendValues.horizontalVelocity;
                                        minimumWidth: 30;
                                        minimum: 0.1
                                        maximum: 10
                                        singleStep: 0.1
                                        baseStateFlag: isBaseState;
                                    }

                                    DoubleSpinBox {
                                        id: verticalVelocitySpinBox;
                                        objectName: "verticalVelocitySpinBox";
                                        backendValue: backendValues.verticalVelocity;
                                        minimumWidth: 30;
                                        minimum: 0.1
                                        maximum: 10
                                        singleStep: 0.1
                                        baseStateFlag: isBaseState;
                                    }

                                    DoubleSpinBox {
                                        id: maximumVelocitySpinBox;
                                        objectName: "maximumVelocitySpinBox";
                                        backendValue: backendValues.maximumFlickVelocity;
                                        minimumWidth: 30;
                                        minimum: 0.1
                                        maximum: 10
                                        singleStep: 0.1
                                        baseStateFlag: isBaseState;
                                    }

                                    CheckBox {
                                        id: overshootCheckBox;
                                        text: "overshoot";
                                        backendValue: backendValues.overShoot;
                                        baseStateFlag: isBaseState;
                                        checkable: true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
