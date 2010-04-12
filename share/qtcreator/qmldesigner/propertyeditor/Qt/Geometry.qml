import Qt 4.6
import Bauhaus 1.0

GroupBox {
    finished: finishedNotify
    id: geometry

    caption: qsTr("Geometry")

    layout: VerticalLayout {

        QWidget {  // 1
            layout: HorizontalLayout {

                Label {
                    text: qsTr("Position")
                }

                DoubleSpinBox {
                    id: xSpinBox;
                    text: "X"
                    alignRight: false
                    spacing: 4
                    singleStep: 1;
                    objectName: "xSpinBox";
                    enabled: anchorBackend.hasParent && !anchorBackend.leftAnchored && !anchorBackend.horizontalCentered
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
                    enabled: anchorBackend.hasParent && !anchorBackend.topAnchored && !anchorBackend.verticalCentered
                    backendValue: backendValues.y
                    minimum: -2000;
                    maximum: 2000;
                    baseStateFlag: isBaseState;
                }


            }
        } //QWidget  //1

        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: qsTr("Size")
                }

                DoubleSpinBox {
                    id: widthSpinBox;
                    text: "W"
                    alignRight: false
                    spacing: 4
                    singleStep: 1;
                    enabled: !anchorBackend.rightAnchored && !anchorBackend.horizontalCentered
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
                    enabled: !anchorBackend.bottomAnchored && !anchorBackend.verticalCentered
                    backendValue: backendValues.height
                    minimum: -2000;
                    maximum: 2000;
                    baseStateFlag: isBaseState;
                }
            } //QVBoxLayout
        } //QWidget
    } //QHBoxLayout
} //QGroupBox
