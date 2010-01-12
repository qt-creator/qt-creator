import Qt 4.6
import Bauhaus 1.0

QWidget {
    id: IntEditor;

    property var backendValue;
    property var baseStateFlag;

    property var caption;

    property var maximumValue: 99
    property var minimumValue: 0
    property var step: 1

    layout: QHBoxLayout {
        topMargin: 2;
        bottomMargin: 2;
        leftMargin: 0;
        rightMargin: 0;
        QLabel {
            text: caption
            font.bold: true;
        }

        SpinBox {
            backendValue:  IntEditor.backendValue;
            minimum: minimumValue;
            maximum: maximumValue;
            baseStateFlag: IntEditor.baseStateFlag;
        }


        QSlider {
            property alias backendValue: IntEditor.backendValue
            orientation: "Qt::Horizontal";
            minimum: minimumValue
            maximum: maximumValue
            singleStep: step

            value: (backendValue == undefined
                    || backendValue == null
                    || backendValue.value == undefined
                    || backendValue.value == null) ? 0 : backendValue.value
            onValueChanged: {
                if (backendValue != undefined && backendValue != null)
                    backendValue.value = value;
            }
        }
    }
}
