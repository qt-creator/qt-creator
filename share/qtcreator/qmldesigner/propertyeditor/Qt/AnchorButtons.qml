import Qt 4.6
import Bauhaus 1.0


QWidget {
    id: anchorButtons
    fixedHeight: 32


    Script {
        function isBorderAnchored() {
            return anchorBackend.leftAnchored || anchorBackend.topAnchored || anchorBackend.rightAnchored || anchorBackend.bottomAnchored;
        }

        function fill() {
            anchorBackend.fill();
        }

        function breakLayout() {
            anchorBackend.resetLayout()
        }
    }

    QPushButton {

        checkable: true
        fixedWidth: 32
        fixedHeight: 32
        styleSheetFile: "anchortop.css";

        checked: anchorBackend.topAnchored;
        onToggled: {
            if (checked) {
                anchorBackend.verticalCentered = false;
                anchorBackend.topAnchored = true;
            } else {
                anchorBackend.topAnchored = false;
            }
        }

    }

    QPushButton {

        x: 32
        checkable: true
        fixedWidth: 32
        fixedHeight: 32

        styleSheetFile: "anchorbottom.css";

        checked: anchorBackend.bottomAnchored;
        onToggled: {
            if (checked) {
                anchorBackend.verticalCentered = false;
                anchorBackend.bottomAnchored = true;
            } else {
                anchorBackend.bottomAnchored = false;
            }
        }

    }
    QPushButton {

        x: 64
        checkable: true
        fixedWidth: 32
        fixedHeight: 32

        styleSheetFile: "anchorleft.css";

        checked: anchorBackend.leftAnchored;
        onToggled: {
            if (checked) {
                    anchorBackend.horizontalCentered = false;
                    anchorBackend.leftAnchored = true;
                } else {
                    anchorBackend.leftAnchored = false;
                }
        }
    }

    QPushButton {

        x: 96
        checkable: true
        fixedWidth: 32
        fixedHeight: 32

        styleSheetFile: "anchorright.css";

        checked: anchorBackend.rightAnchored;
        onToggled: {
            if (checked) {
                    anchorBackend.horizontalCentered = false;
                    anchorBackend.rightAnchored = true;
                } else {
                    anchorBackend.rightAnchored = false;
                }
        }
    }

    QPushButton {
        x: 128
        checkable: true
        fixedWidth: 32
        fixedHeight: 32

        styleSheetFile: "anchorspacer.css";


    }

    QPushButton {
        x: 128 + 21
        fixedWidth: 32
        fixedHeight: 32

        styleSheetFile: "anchorfill.css";

        onReleased: fill();
    }

    QPushButton {
        x: 128 + 21 + 32
        checkable: true
        fixedWidth: 32
        fixedHeight: 32

        styleSheetFile: "anchorspacer.css";

    }

    QPushButton {
        x: 128 + 42 + 64
        checkable: true
        fixedWidth: 32
        fixedHeight: 32

        styleSheetFile: "anchorhorizontal.css";

        checked: anchorBackend.horizontalCentered;
        onToggled: {
            if (checked) {
                    anchorBackend.leftAnchored = false;
                    anchorBackend.rightAnchored = false;
                    anchorBackend.horizontalCentered = true;
            } else {
                    anchorBackend.horizontalCentered = false;
            }
        }
    }

    QPushButton {
        x: 128 + 42 + 32
        checkable: true
        fixedWidth: 32
        fixedHeight: 32

        styleSheetFile: "anchorvertical.css";

        checked: anchorBackend.verticalCentered;
        onToggled: {
            if (checked) {
                    anchorBackend.topAnchored = false;
                    anchorBackend.bottomAnchored = false;
                    anchorBackend.verticalCentered = true;
            } else {
                    anchorBackend.verticalCentered = false;
            }
        }
    }
}

