import Qt 4.6
import Bauhaus 1.0

GroupBox {
    finished: finishedNotify;
    id: Geometry;
    caption: "Geometry";

    layout: VerticalLayout {        

        QWidget {  // 1
            layout: HorizontalLayout {

                Label {
                    text: "Position"
                }

                DoubleSpinBox {
                    id: XSpinBox;
                    text: "X"
					alignRight: false
					spacing: 4
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
					alignRight: false
					spacing: 4
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

                Label {
                    text: "Size"
                }

                DoubleSpinBox {
                    id: WidthSpinBox;
                    text: "W"
					alignRight: false
					spacing: 4
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
					alignRight: false
					spacing: 4
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
