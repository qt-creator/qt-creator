import Qt 4.6

GroupBox {
    finished: finishedNotify;
    id: Geometry;
    caption: "Geometry";

    maximumHeight: 240;

    layout: VerticalLayout {        

        QWidget {  // 1
            layout: HorizontalLayout {

                QLabel {
                    text: "Position"
                    alignment: "Qt::AlignRight | Qt::AlignVCenter"
                }

                DoubleSpinBox {
                    id: XSpinBox;
                    text: "X"
                    singleStep: 1;
                    objectName: "XSpinBox";
                    enabled: anchorBackend.hasParent;
                    backendValue: backendValues.x
                    minimum: -2000;
                    maximum: 2000;
                    baseStateFlag: isBaseState;
                }

                DoubleSpinBox {
                    id: YSpinBox;
                    singleStep: 1;
                    text: "Y"
                    objectName: "YSpinBox";
                    backendValue: backendValues.y
                    enabled: anchorBackend.hasParent;
                    minimum: -2000;
                    maximum: 2000;
                    baseStateFlag: isBaseState;
                }


            }
        } //QWidget  //1

        QWidget {
            layout: HorizontalLayout {				

                QLabel {
                    text: "Size"
                    alignment: "Qt::AlignRight | Qt::AlignVCenter"
                }

                DoubleSpinBox {
                    id: WidthSpinBox;
                    text: "W"
                    singleStep: 1;
                    objectName: "WidthSpinBox";
                    backendValue: backendValues.width
                    minimum: -2000;
                    maximum: 2000;
                    baseStateFlag: isBaseState;
                }

                DoubleSpinBox {
                    id: HeightSpinBox;
                    text: "H"
                    singleStep: 1;
                    objectName: "HeightSpinBox";
                    backendValue: backendValues.height
                    minimum: -2000;
                    maximum: 2000;
                    baseStateFlag: isBaseState;
                }

            } //QVBoxLayout
        } //QWidget

    } //QHBoxLayout
} //QGroupBox
