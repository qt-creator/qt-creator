import Qt 4.6
import Bauhaus 1.0

GroupBox {
    finished: finishedNotify
    id: geometry

    caption: "geometry"

    layout: VerticalLayout {

        QWidget {  // 1
            layout: HorizontalLayout {

                Label {
                    text: "Position"
                }

                DoubleSpinBox {
                    id: xSpinBox;
                    text: "X"
					alignRight: false
					spacing: 4
                    singleStep: 1;
                    objectName: "xSpinBox";
                    enabled: anchorBackend.hasParent;
                    backendValue: backendValues.x
                    minimum: -2000;
                    maximum: 2000;
                    baseStateFlag: isBaseState;
                }

                DoubleSpinBox {
                    id: ySpinBox;
                    singleStep: 1;
                    text: "Y"
					alignRight: false
					spacing: 4
                    objectName: "ySpinBox";
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

                Label {
                    text: "Size"
                }

                DoubleSpinBox {
                    id: widthSpinBox;
                    text: "W"
					alignRight: false
					spacing: 4
                    singleStep: 1;
                    objectName: "widthSpinBox";
                    backendValue: backendValues.width
                    minimum: -2000;
                    maximum: 2000;
                    baseStateFlag: isBaseState;
                }

                DoubleSpinBox {
                    id: heightSpinBox;
                    text: "H"
					alignRight: false
					spacing: 4
                    singleStep: 1;
                    objectName: "heightSpinBox";
                    backendValue: backendValues.height
                    minimum: -2000;
                    maximum: 2000;
                    baseStateFlag: isBaseState;
                }

            } //QVBoxLayout
        } //QWidget

    } //QHBoxLayout
} //QGroupBox
