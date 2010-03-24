import Qt 4.6
import Bauhaus 1.0


QWidget {
    layout: QVBoxLayout {
        topMargin: 0
        bottomMargin: 0
        leftMargin: 0
        rightMargin: 0
        spacing: 0

        RectangleColorGroupBox {
            finished: finishedNotify;
        }

        GroupBox {

            finished: finishedNotify;
            caption: "Rectangle"

            layout: VerticalLayout {
			    rightMargin: 24

                IntEditor {
                    backendValue: backendValues.radius
                    caption: "Radius"
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: Math.max(1,(Math.min(backendValues.width.value,backendValues.height.value)-backendValues.border_width.value)/2)
                }


                IntEditor {
                    id: borderWidth;
                    backendValue: backendValues.border_width === undefined ? 0 : backendValues.border_width

                    caption: "Border"
                    baseStateFlag: isBaseState;

                    step: 1;
                    minimumValue: 0;
                    maximumValue: 100;
                }
            }
        }

        QScrollArea {
        }
    }
}
