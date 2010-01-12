import Qt 4.6
import Bauhaus 1.0

QExtGroupBox {
    id: GroupBox;

    property var finished;

    property var caption;

    property var oldMaximumHeight;

    onFinishedChanged: {
        CheckBox.raise();
        maximumHeight = height;
        oldMaximumHeight = maximumHeight;
        visible = false;
        visible = true;
        x = 6;
    }

    QToolButton {
        //QCheckBox {
        id: CheckBox;
        text: "";
        focusPolicy: "Qt::NoFocus";
        styleSheetFile: "specialCheckBox.css";
        y: 4;
        x: 6;
        height: 12;
        width: 12;
        arrowType: "Qt::DownArrow";
        toolButtonStyle: "Qt::ToolButtonTextBesideIcon";
        checkable: true;
        checked: true;
        font.bold: true;
        onClicked: {
            if (checked) {
                //GroupBox.maximumHeight = oldMaximumHeight;
                collapsed = false;
                text = "";
                width = 12;
                x = 6;
                arrowType =  "Qt::DownArrow";
                } else {
                //GroupBox.maximumHeight = 20;

                collapsed = true;
                text = GroupBox.caption;
                visible = true;
                width = 120;
                x = 2;
                arrowType =  "Qt::RightArrow";
                }
        }
    }
}
