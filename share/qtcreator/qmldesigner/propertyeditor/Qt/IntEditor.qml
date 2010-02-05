import Qt 4.6
import Bauhaus 1.0

QWidget {
    id: intEditor;

    property var backendValue;
    property var baseStateFlag;

    property var caption;

    property var maximumValue: 99
    property var minimumValue: 0
    property var step: 1
    property bool slider: true
    property alias alignment: label.alignment

    layout: HorizontalLayout {
        Label {
            id: label
            text: caption        
        }

        SpinBox {
            backendValue: (intEditor.backendValue === undefined ||
                           intEditor.backendValue === null)
            ? null : intEditor.backendValue;

            minimum: minimumValue
            maximum: maximumValue
            baseStateFlag: (intEditor.backendValue === undefined ||
                            intEditor.backendValue === null)
            ? null : intEditor.baseStateFlag;
        }


        QWidget {
            visible: intEditor.slider
            id: sliderWidget
            QSlider {
                y: sliderWidget.height / 2 - 12
                width: sliderWidget.width
                height: 24
                property alias backendValue: intEditor.backendValue
                orientation: "Qt::Horizontal"
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
}
