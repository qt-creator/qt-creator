import Qt 4.7
import Bauhaus 1.0

GroupBox {
    finished: finishedNotify
    id: geometry

    caption: qsTr("Geometry")

    layout: VerticalLayout {
        bottomMargin: 12

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
            id: bottomWidget
            layout: HorizontalLayout {

                Label {
                    id: sizeLabel
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
    
    QPushButton {
        checkable: true
        toolTip: qsTr("Lock aspect ratio")
        fixedWidth: 45
		width: fixedWidth
        fixedHeight: 9
		height: fixedHeight
        styleSheetFile: "aspectlock.css";
        x: bottomWidget.x + ((bottomWidget.width - sizeLabel.width) / 2) + sizeLabel.width - 8
        y: bottomWidget.y + bottomWidget.height + 2
        visible: false //hide until the visual editor implements this feature ###
    }
} //QGroupBox
