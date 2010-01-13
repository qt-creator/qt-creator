import Qt 4.6

GroupBox {
    finished: finishedNotify;
    id: Geometry;
    caption: "Geometry";

    maximumHeight: 240;

    layout: QHBoxLayout {
        topMargin: 14;
        bottomMargin: 4;
        leftMargin: 4;
        rightMargin: 4;

        QWidget {
            layout: QVBoxLayout {
                topMargin: 0;
                bottomMargin: 60;
                leftMargin: 10;
                rightMargin: 10;
                spacing: 25;

                QLabel {
                    text: "Position:";
                    font.bold: true;
                }
                QLabel {
                    text: "Size:"
                    font.bold: true
                }
            }
        }

        QWidget {
            layout: QVBoxLayout {
                topMargin: 0;
                bottomMargin: 0;
                QWidget {
                    layout: QHBoxLayout {
                        topMargin: 0;
                        bottomMargin: 0;

                        //---

                        QWidget {  // 1
                            layout: QHBoxLayout { //2
                                topMargin: 0;
                                bottomMargin: 0;
                                QWidget { //3
                                    layout: QVBoxLayout { //4
                                        topMargin: 0;
                                        bottomMargin: 0;
                                        DoubleSpinBox {
                                            id: XSpinBox;
                                            singleStep: 1;
                                            objectName: "XSpinBox";
                                            enabled: anchorBackend.hasParent;
                                            backendValue: backendValues.x == undefined ? 0 : backendValues.x
                                            minimum: -2000;
                                            maximum: 2000;
                                            baseStateFlag: isBaseState;
                                        }
                                        QLabel { //6
                                            text: "X";
                                            alignment: "Qt::AlignHCenter"
                                            font.bold: true;
                                        } // 6
                                    } //QHBoxLayout //4
                                } //QWidget //3
                                QWidget { // 7
                                    layout: QVBoxLayout { //8
                                        topMargin: 0;
                                        bottomMargin: 0;

                                        DoubleSpinBox {
                                            id: YSpinBox;
                                            singleStep: 1;
                                            objectName: "YSpinBox";
                                            backendValue: backendValues.y == undefined ? 0 : backendValues.y
                                            enabled: anchorBackend.hasParent;
                                            minimum: -2000;
                                            maximum: 2000;
                                            baseStateFlag: isBaseState;
                                        }
                                        QLabel { //10
                                            text: "Y";
                                            alignment: "Qt::AlignHCenter";
                                            font.bold: true;
                                        } //10
                                    } //QVBoxLayout //8
                                } // QWidget //3

                            } //QHBoxLayout //7
                        } //QWidget  //1
                        //---
                    } //QHBoxLayout
                } //GroupBox
                QWidget {
                    layout: QHBoxLayout {
                        topMargin: 0;
                        bottomMargin: 10;
                        //---
                        QWidget {
                            layout: QHBoxLayout {
                                topMargin: 0;
                                bottomMargin: 0;
                                QWidget {
                                    layout: QVBoxLayout {
                                        topMargin: 0;
                                        bottomMargin: 0;
                                        DoubleSpinBox {
                                            id: WidthSpinBox;
                                            singleStep: 1;
                                            objectName: "WidthSpinBox";
                                            backendValue: backendValues.width == undefined ? 0 : backendValues.width
                                            minimum: -2000;
                                            maximum: 2000;
                                            baseStateFlag: isBaseState;
                                        }
                                        QLabel {
                                            text: "Width";
                                            alignment: "Qt::AlignHCenter";
                                            font.bold: true;
                                        }
                                    } //QVBoxLayout
                                } //QWidget
                                QWidget {
                                    layout: QVBoxLayout {
                                        topMargin: 0;
                                        bottomMargin: 0;
                                        DoubleSpinBox {
                                            id: HeightSpinBox;
                                            singleStep: 1;
                                            objectName: "HeightSpinBox";
                                            backendValue: backendValues.height == undefined ? 0 : backendValues.height
                                            minimum: -2000;
                                            maximum: 2000;
                                            baseStateFlag: isBaseState;
                                        }
                                        QLabel {
                                            id: HeightLabel;
                                            text: "Height";
                                            alignment: "Qt::AlignHCenter";
                                            font.bold: true;
                                        }
                                    } //QVBoxLayout
                                } //QWidget
                            } //QHBoxLayout
                        } //QWidget
                    } // QHBoxLayout
                } //Widget

                IntEditor {
                    id: borderWidth;
                    backendValue: backendValues.z == undefined ? 0 : backendValues.z
                    caption: "z:"
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 100;
                }
            } //QVBoxLayout
        } //QWidget
    } //QHBoxLayout
} //QGroupBox
