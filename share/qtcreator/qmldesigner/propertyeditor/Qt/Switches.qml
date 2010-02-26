import Qt 4.6
import Bauhaus 1.0

QFrame {
    styleSheetFile: "switch.css";
    property var specialModeIcon;
    specialModeIcon: "images/standard.png";
    maximumWidth: 300;
    minimumWidth: 300;
    layout: QHBoxLayout {
        topMargin: 0;
        bottomMargin: 0;
        leftMargin: 0;
        rightMargin: 40;
        spacing: 0;

        QPushButton {
            checkable: true;
            checked: true;
            id: standardMode;
            toolTip: "general item properties";
            //iconFromFile: "images/rect-icon.png";
            text: "Common"
            onClicked: {
                extendedMode.checked = false;
                layoutMode.checked = false;
                checked = true;
                standardPane.visible = true;
                extendedPane.visible = false;
                layoutPane.visible = false;
            }
        }
        QPushButton {
            id: extendedMode;
            toolTip: "extended properties";
            checkable: true;
            checked: false;
            text: "Advanced"
            onClicked: {
                standardMode.checked = false;
                layoutMode.checked = false;
                checked = true;
                standardPane.visible = false;
                extendedPane.visible = true;
                layoutPane.visible = false;
            }
        }
        QPushButton {
            id: layoutMode;
            checkable: true;
            checked: false;
            toolTip: "layout properties";
            text: "Anchor";
            onClicked: {
                extendedMode.checked = false;
                standardMode.checked = false;
                checked = true;
                standardPane.visible = false;
                extendedPane.visible = false;
                layoutPane.visible = true;
            }
        }
    }
}
