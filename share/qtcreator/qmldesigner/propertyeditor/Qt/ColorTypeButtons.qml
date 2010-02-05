import Qt 4.6
import Bauhaus 1.0

QGroupBox {
    id: colorTypeButtons
    layout: HorizontalLayout {
        topMargin: 6
        rightMargin: 10;
        Label {
            text: "Type"
        }

        QWidget {
            fixedHeight: 32

            QPushButton {
                id: noneButton
                checkable: true
                fixedWidth: 32
                fixedHeight: 32
                styleSheetFile: "nonecolorbutton.css";

                onToggled: {
                    if (checked) {
                        gradientButton.checked = false;
                        solidButton.checked = false;
                    }
                }

            }
            QPushButton {
                id: solidButton
                x: 32
                checkable: true
                fixedWidth: 32
                fixedHeight: 32

                styleSheetFile: "solidcolorbutton.css";

                onToggled: {
                    if (checked) {
                        gradientButton.checked = false;
                        noneButton.checked = false;
                    }
                }

            }
            QPushButton {
                id: gradientButton
                x: 64
                checkable: true
                fixedWidth: 32
                fixedHeight: 32

                styleSheetFile: "gradientcolorbutton.css";

                onToggled: {
                    if (checked) {
                        solidButton.checked = false;
                        noneButton.checked = false;
                    }
                }
            }
        }

    }

}
