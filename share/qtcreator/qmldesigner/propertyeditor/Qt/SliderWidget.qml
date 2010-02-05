import Qt 4.6
import Bauhaus 1.0

QWidget {
    id: sliderWidget

    property var value
    property alias singleStep: localSlider.singleStep
    property alias minimum: localSlider.minimum
    property alias maximum: localSlider.maximum

    QSlider {
        orientation: "Qt::Horizontal";
        id: localSlider
        y: sliderWidget.height / 2 - 12
        width: sliderWidget.width
        height: 24
		value: sliderWidget.value
		onValueChanged: {
			sliderWidget.value = value	
		}
    }
}
