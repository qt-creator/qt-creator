import Qt 4.6

QFrame {
    styleSheetFile: "switch.css";
    property var specialModeIcon;
    specialModeIcon: "images/standard.png";
    maximumWidth: 286;
    minimumWidth: 286;
    layout: QHBoxLayout {
        topMargin: 4;
        bottomMargin: 0;
        leftMargin: 4;
        rightMargin: 80;
        spacing: 0;

        QPushButton {
            checkable: true;
            checked: true;
            id: standardMode;
            toolTip: "general item properties";
            iconFromFile: "images/rect-icon.png";
            onClicked: {
                extendedMode.checked = false;
                layoutMode.checked = false;
                specialMode.checked = false;
                checked = true;
                standardPane.visible = true;
                extendedPane.visible = false;
                layoutPane.visible = false;
                specialPane.visible = false;
            }
        }
        QPushButton {
            checkable: true;
            checked: false;
            id: specialMode;
            toolTip: "type specific properties";
            iconFromFile: specialModeIcon;
            onClicked: {
                extendedMode.checked = false;
                standardMode.checked = false;
                layoutMode.checked = false;
                checked = true;
                specialPane.visible = true;
                standardPane.visible = false;
                extendedPane.visible = false;
                layoutPane.visible = false;
            }
        }
        QPushButton {
            id: extendedMode;
            toolTip: "extended properties";
            checkable: true;
            checked: false;
            iconFromFile: "images/extended.png";
            onClicked: {
                standardMode.checked = false;
                layoutMode.checked = false;
                specialMode.checked = false;
                checked = true;
                standardPane.visible = false;
                extendedPane.visible = true;
                layoutPane.visible = false;
                specialPane.visible = false;
            }
        }
        QPushButton {
            id: layoutMode;
            checkable: true;
            checked: false;
            toolTip: "layout properties";
            iconFromFile: "images/layout.png";
            onClicked: {
                extendedMode.checked = false;
                standardMode.checked = false;
                specialMode.checked = false;
                checked = true;
                standardPane.visible = false;
                extendedPane.visible = false;
                layoutPane.visible = true;
                specialPane.visible = false;
            }
        }
    }
}
