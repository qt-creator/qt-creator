import Qt 4.7
import Bauhaus 1.0

QFrame {
    styleSheetFile: "switch.css";
    property variant specialModeIcon;
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
            toolTip: qsTr("special properties");
            //iconFromFile: "images/rect-icon.png";
            text: backendValues === undefined || backendValues.className === undefined || backendValues.className == "empty" ? "empty" : backendValues.className.value
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
            id: layoutMode;
            checkable: true;
            checked: false;
            toolTip: qsTr("layout and geometry");
            text: qsTr("Geometry");
            onClicked: {
                extendedMode.checked = false;
                standardMode.checked = false;
                checked = true;
                standardPane.visible = false;
                extendedPane.visible = false;
                layoutPane.visible = true;
            }
        }

        QPushButton {
            id: extendedMode;
            toolTip: qsTr("advanced properties");
            checkable: true;
            checked: false;
            text: qsTr("Advanced")
            onClicked: {
                standardMode.checked = false;
                layoutMode.checked = false;
                checked = true;
                standardPane.visible = false;
                extendedPane.visible = true;
                layoutPane.visible = false;
            }
        }

    }
}
