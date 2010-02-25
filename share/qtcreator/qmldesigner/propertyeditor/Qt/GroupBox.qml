import Qt 4.6
import Bauhaus 1.0

QExtGroupBox {
    id: groupBox;

    property var finished;

    property var caption;

    property var oldMaximumHeight;

    onFinishedChanged: {
        checkBox.raise();
        oldMaximumHeight = maximumHeight;
        visible = false;
        visible = true;
        //x = 6;
    }

    QToolButton {
        //QCheckBox {
        id: checkBox;

        text: groupBox.caption;
        focusPolicy: "Qt::NoFocus";
        styleSheetFile: "specialCheckBox.css";
        y: 0;		
        x: 0;
        fixedHeight: 17
        fixedWidth: groupBox.width;
        arrowType: "Qt::DownArrow";
        toolButtonStyle: "Qt::ToolButtonTextBesideIcon";
        checkable: true;
        checked: true;
        font.bold: true;
        onClicked: {
            if (checked) {
                //groupBox.maximumHeight = oldMaximumHeight;
                collapsed = false;
                //text = "";
                //width = 12;
				//width = 120;
                arrowType =  "Qt::DownArrow";
				visible = true;
                } else {
                //groupBox.maximumHeight = 20;

                collapsed = true;
                //text = groupBox.caption;
                visible = true;
                //width = 120;
                arrowType =  "Qt::RightArrow";
                }
        }
    }
}
