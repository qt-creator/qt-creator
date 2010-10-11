import Qt 4.7
import Bauhaus 1.0


QWidget {
    layout: QVBoxLayout {
        topMargin: 0
        bottomMargin: 0
        leftMargin: 0
        rightMargin: 0
        spacing: 0

        RectangleColorGroupBox {
            id: colorsBox
            finished: finishedNotify;
        }

        GroupBox {

            finished: finishedNotify;
            caption: qsTr("Rectangle")

            layout: VerticalLayout {
                rightMargin: 24

                IntEditor {
                    visible: colorsBox.hasBorder
                    id: borderWidth;
                    backendValue: backendValues.border_width === undefined ? 0 : backendValues.border_width

                    caption: qsTr("Border")
                    baseStateFlag: isBaseState;

                    step: 1;
                    minimumValue: 0;
                    maximumValue: 100;
                }

                IntEditor {
                    property int border: backendValues.border_width.value === undefined ? 0 : backendValues.border_width.value
                    backendValue: backendValues.radius
                    caption: qsTr("Radius")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: Math.max(1, (Math.min(backendValues.width.value,backendValues.height.value) - border) / 2)
                }

            }
        }

        QScrollArea {
        }
    }
}
