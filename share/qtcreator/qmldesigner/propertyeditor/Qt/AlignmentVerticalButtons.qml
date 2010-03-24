import Qt 4.6
import Bauhaus 1.0

QGroupBox {
    id: alignmentVerticalButtons
    layout: HorizontalLayout {
        topMargin: 6

        QWidget {
            fixedHeight: 32

            QPushButton {
                id: topButton;
                checkable: true
                fixedWidth: 32
                fixedHeight: 32
                styleSheetFile: "alignmenttopbutton.css";
                checked: backendValues.verticalAlignment.value == "AlignTop"

                onClicked: {
                    backendValues.verticalAlignment.value = "AlignTop";
                    checked = true;
                    bottomButton.checked = false;
                    centerButton.checked = false;
                }

            }
            QPushButton {
                x: 32
                id: centerButton;
                checkable: true
                fixedWidth: 32
                fixedHeight: 32

                styleSheetFile: "alignmentcentervbutton.css";
                checked: backendValues.verticalAlignment.value == "AlignVCenter"

                onClicked: {
                    backendValues.verticalAlignment.value = "AlignVCenter";
                    checked = true;
                    bottomButton.checked = false;
                    topButton.checked = false;
                }

            }
            QPushButton {
                x: 64
                id: bottomButton;
                checkable: true
                fixedWidth: 32
                fixedHeight: 32

                styleSheetFile: "alignmentbottombutton.css";
                checked: backendValues.verticalAlignment.value == "AlignBottom"

                onClicked: {
                    backendValues.verticalAlignment.value = "AlignBottom";
                    checked = true;
                    centerButton.checked = false;
                    topButton.checked = false;
                }
            }
        }

    }

}
