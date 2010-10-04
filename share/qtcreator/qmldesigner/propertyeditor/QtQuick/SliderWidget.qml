import Qt 4.7
import Bauhaus 1.0

QWidget {
    id: sliderWidget

    property variant value
    property alias singleStep: localSlider.singleStep
    property alias minimum: localSlider.minimum
    property alias maximum: localSlider.maximum
    property variant backendValue

    QSlider {
        orientation: "Qt::Horizontal";
        id: localSlider
        y: sliderWidget.height / 2 - 12
        width: sliderWidget.width
        height: 24
        value: sliderWidget.value
        onValueChanged: {        
            if (sliderWidget.value != value)
                sliderWidget.value = value
        }

        onSliderPressed: {
            transaction.start();
        }
        onSliderReleased: {
            transaction.end();
        }
    }
}
