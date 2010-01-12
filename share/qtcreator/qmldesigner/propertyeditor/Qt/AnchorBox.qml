import Qt 4.6

QWidget {
    width:220;fixedWidth:  width
    height:220;fixedHeight: height
    styleSheetFile: "anchorbox.css"

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
	text: "fill";
        height:20;fixedHeight: height;
        x: 0;
        y: 0;
        width:200;fixedWidth: width;
        id: QPushButton1;
	onReleased: fill();
    }

    QPushButton {
    text: "break";
	y: 200;
	height:20;fixedHeight: height;
	width:200;fixedWidth: width;
	x: 0;
	id: QPushButton3;
	onReleased: breakLayout();
    }

    QPushButton {
        height:100;fixedHeight: height;
	text: "left";
	font.bold: true;
        x: 16 ;
        y: 60;
        //styleSheet: "border-radius:5px; background-color: #ffda82";
        width:30;fixedWidth: width;
        id: QPushButton5;
	checkable: true;
	checked: anchorBackend.leftAnchored;
	onReleased: {
	      if (checked) {
		anchorBackend.horizontalCentered = false;
		anchorBackend.leftAnchored = true;
	      } else {
		anchorBackend.leftAnchored = false;
	      }
	}
    }

    QPushButton {
	text: "top";
	font.bold: true;
        //styleSheet: "border-radius:5px; background-color: #ffda82";
        height:27;fixedHeight: 27;
        width:100;fixedWidth: 100;
        x: 49;
        y: 30;
        id: QPushButton6;
	checkable: true;
	checked: anchorBackend.topAnchored;
	onReleased: {
	      if (checked) {
		anchorBackend.verticalCentered = false;
		anchorBackend.topAnchored = true;
	      } else {
		anchorBackend.topAnchored = false;
	      }
	}
    }

    QPushButton {
	text: "right";
	font.bold: true;
	x: 153;
	y: 60;
	//styleSheet: "border-radius:5px; background-color: #ffda82";
	width:30;fixedWidth: width;
	height:100;fixedHeight: height;
	id: QPushButton7;
	checkable: true;
	checked: anchorBackend.rightAnchored;
	onReleased: {
	      if (checked) {
		anchorBackend.horizontalCentered = false;
		anchorBackend.rightAnchored = true;
	      } else {
		anchorBackend.rightAnchored = false;
	      }
	}
    }

    QPushButton {
	text: "bottom";
	font.bold: true;
        //styleSheet: "border-radius:5px; background-color: #ffda82";
        width:100;fixedWidth: width;
        x: 49;
        y: 164;
        height:27;fixedHeight: height;
        id: QPushButton8;
	checkable: true;
	checked: anchorBackend.bottomAnchored;
	onReleased: {
	      if (checked) {
		anchorBackend.verticalCentered = false;
		anchorBackend.bottomAnchored = true;
	      } else {
		anchorBackend.bottomAnchored = false;
	      }
	}
    }

    QToolButton {
        width:100;fixedWidth: width;
        //styleSheet: "border-radius:50px;background-color: rgba(85, 170, 255, 255)";
        x: 49;
        y: 60;
        height:100;fixedHeight: height;
        id: QPushButton9;

	QPushButton {
	    width:24;fixedWidth: width;
	    //styleSheet: "border-radius:5px; background-color: #bf3f00;";
	    x: 38;
	    y: 2;
	    height:96;fixedHeight: height;
	    checkable: true;
	    id: horizontalCenterButton;
	    checked: anchorBackend.horizontalCentered;
	    onReleased: {
		if (checked) {
		  anchorBackend.rightAnchored = false;
		  anchorBackend.leftAnchored = false;
		  anchorBackend.horizontalCentered = true;
		} else {
		  anchorBackend.horizontalCentered = false;
		}
	    }
	}

	QPushButton {
	    height:24;fixedHeight: height;
	    x: 2;
	    y: 38;
	    width:96;fixedWidth: width;
	    id: verticalCenterButton;
	    checkable: true;
	    checked: anchorBackend.verticalCentered;
	    onReleased: {
		if (checked) {
		  anchorBackend.topAnchored = false;
		  anchorBackend.bottomAnchored = false;
		  anchorBackend.verticalCentered = true;
		} else {
		  anchorBackend.verticalCentered = false;
		}
	    }
	}

	QPushButton {
	    text: "center";
	    font.bold: true;
	    //styleSheet: "border-radius:20px; background-color: #ff5500";
	    width:40;fixedWidth: width;
	    height:40;fixedHeight: height;
	    x: 30;
	    y: 30;
	    id: centerButton;
	    checkable: true;
	    checked: anchorBackend.verticalCentered && anchorBackend.horizontalCentered;
	    onReleased: {
                if (checked) {
		    anchorBackend.leftAnchored = false;
		    anchorBackend.topAnchored = false;
		    anchorBackend.rightAnchored = false;
		    anchorBackend.bottomAnchored = false;
		    anchorBackend.verticalCentered = true;
		    anchorBackend.horizontalCentered = true;
                } else {
		    anchorBackend.verticalCentered = false;
		    anchorBackend.horizontalCentered = false;
                }
	    }
	}
    }
}
