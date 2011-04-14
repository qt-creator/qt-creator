import Qt 4.7
import Bauhaus 1.0

QWidget {
    id: intEditor;

    property variant backendValue;
    property variant baseStateFlag;

    property variant caption;

    property variant maximumValue: 1000
    property variant minimumValue: -1000
    property variant step: 1
    property bool slider: true
    property alias alignment: label.alignment

    layout: HorizontalLayout {
        Label {
            id: label
            text: caption
            toolTip: caption
        }

        SpinBox {
            backendValue: (intEditor.backendValue === undefined ||
            intEditor.backendValue === null)
            ? null : intEditor.backendValue;

            property variant backendValueValue: (intEditor.backendValue === undefined ||
            intEditor.backendValue === null)
            ? null : intEditor.backendValue.value;

            minimum: minimumValue
            maximum: maximumValue
            baseStateFlag: intEditor.baseStateFlag
        }


        QWidget {
            visible: intEditor.slider
            id: sliderWidget
            QSlider {
                id: intSlider
                y: sliderWidget.height / 2 - 12
                width: sliderWidget.width
                height: 24
                property alias backendValue: intEditor.backendValue
                orientation: "Qt::Horizontal"
                minimum: minimumValue
                maximum: maximumValue
                singleStep: step
                property int valueFromBackend: intEditor.backendValue.value;

                onValueFromBackendChanged: {
                    if (intSlider.maximum < valueFromBackend)
                        intSlider.maximum = valueFromBackend;
                    if (intSlider.minimum > valueFromBackend)
                        intSlider.minimum = valueFromBackend;
                    intSlider.value = valueFromBackend;
                }

                onValueChanged: {
                    if (backendValue != undefined && backendValue != null)
                    backendValue.value = value;
                }

                onSliderPressed: {
                    transaction.start();
                }
                onSliderReleased: {
                    transaction.end();
                }
            }
        }
    }
}
