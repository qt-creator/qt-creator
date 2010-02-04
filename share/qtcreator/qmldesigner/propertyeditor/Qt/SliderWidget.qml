import Qt 4.6
import Bauhaus 1.0

QWidget {
    id: SliderWidget

    property var value
    property alias singleStep: LocalSlider.singleStep
    property alias minimum: LocalSlider.minimum
    property alias maximum: LocalSlider.maximum

    QSlider {
        orientation: "Qt::Horizontal";
        id: LocalSlider
        y: SliderWidget.height / 2 - 12
        width: SliderWidget.width
        height: 24
		value: SliderWidget.value
		onValueChanged: {
			SliderWidget.value = value	
		}
    }
}
