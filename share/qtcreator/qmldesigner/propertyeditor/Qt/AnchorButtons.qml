import Qt 4.6
import Bauhaus 1.0


QWidget {
    id: anchorButtons
    fixedHeight: 32

    function isBorderAnchored() {
        return anchorBackend.leftAnchored || anchorBackend.topAnchored || anchorBackend.rightAnchored || anchorBackend.bottomAnchored;
    }

    function fill() {
        anchorBackend.fill();
    }

    function breakLayout() {
        anchorBackend.resetLayout()
    }

    QPushButton {

        checkable: true
        fixedWidth: 31
		width: fixedWidth
        fixedHeight: 28
		height: fixedHeight
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

        x: 31
        checkable: true
        fixedWidth: 30
		width: fixedWidth
        fixedHeight: 28
		height: fixedHeight

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

        x: 61
        checkable: true
        fixedWidth: 30
		width: fixedWidth
        fixedHeight: 28
		height: fixedHeight

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

        x: 91
        checkable: true
        fixedWidth: 30
		width: fixedWidth
        fixedHeight: 28
		height: fixedHeight

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
        x: 121
        checkable: true
        fixedWidth: 19
		width: fixedWidth
        fixedHeight: 28
		height: fixedHeight

        styleSheetFile: "anchorspacer.css";


    }

    QPushButton {
        x: 140
        fixedWidth: 30
        fixedHeight: 28
		height: fixedHeight

        styleSheetFile: "anchorfill.css";

        onReleased: fill();
    }

    QPushButton {
        x: 170
        checkable: true
        fixedWidth: 19
		width: fixedWidth
        fixedHeight: 28
		height: fixedHeight

        styleSheetFile: "anchorspacer.css";

    }

    QPushButton {
        x: 189
        checkable: true
        fixedWidth: 30
		width: fixedWidth
        fixedHeight: 28
		height: fixedHeight

		styleSheetFile: "anchorvertical.css";

        checked: anchorBackend.verticalCentered;
        onToggled: {
            if (checked) {
                    anchorBackend.leftAnchored = false;
                    anchorBackend.rightAnchored = false;
					anchorBackend.topAnchored = false;
                    anchorBackend.bottomAnchored = false;
                    anchorBackend.verticalCentered = true;
            } else {
                    anchorBackend.verticalCentered = false;
            }
        }
    }

    QPushButton {
        x: 219
        checkable: true
        fixedWidth: 31
		width: fixedWidth
        fixedHeight: 28
		height: fixedHeight
        
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
}

