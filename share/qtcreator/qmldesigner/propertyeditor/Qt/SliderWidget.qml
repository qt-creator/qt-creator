import Qt 4.6
import Bauhaus 1.0

QWidget {
    id: SliderWidget

    property alias value: slider.value
    property alias singleStep: slider.singleStep
    property alias minimum: slider.minimum
    property alias maximum: slider.maximum

    QSlider {
        orientation: "Qt::Horizontal";
        id: slider
        y: SliderWidget.height / 2 - 12
        width: SliderWidget.width
        height: 24
    }
}
