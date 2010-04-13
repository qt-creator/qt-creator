import Qt 4.7
import Bauhaus 1.0

QExtGroupBox {
    id: groupBoxOption;

    property variant finished;

    property variant caption;

    property variant oldMaximumHeight;

    onFinishedChanged: {
        CheckBox.raise();
        maximumHeight = height;
        oldMaximumHeight = maximumHeight;
        minimumHeight = height;
        x = 6;
        visible = false;
    }
}
