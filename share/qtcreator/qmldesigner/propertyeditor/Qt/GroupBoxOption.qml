import Qt 4.6
import Bauhaus 1.0

QExtGroupBox {
    id: groupBoxOption;

    property var finished;

    property var caption;

    property var oldMaximumHeight;

    onFinishedChanged: {
        CheckBox.raise();
        maximumHeight = height;
        oldMaximumHeight = maximumHeight;
        minimumHeight = height;
        x = 6;
        visible = false;
    }
}
