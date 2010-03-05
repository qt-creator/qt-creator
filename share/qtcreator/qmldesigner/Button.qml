import Qt 4.6

Rectangle {
    property var label: "Button"
    signal clicked

    width: 75
    height: 20
    radius: 10
    color: "grey"

    Rectangle {
	  anchors.fill: parent;
	  anchors.leftMargin: 1;
	  anchors.rightMargin: 1;
	  anchors.topMargin: 1;
	  anchors.bottomMargin: 1;

	  color: "#2c2c2c";
	  radius: 9

	  Rectangle {
          id: buttonGradientRectangle
		  anchors.fill: parent;
		  anchors.leftMargin: 1;
		  anchors.rightMargin: 1;
		  anchors.topMargin: 1;
		  anchors.bottomMargin: 1;

		  color: "black";
		  gradient: normalGradient
		  radius: 8;

          Gradient {
              id: pressedGradient
              GradientStop { position: 0.0; color: "#686868" }
              GradientStop { position: 1.0; color: "#8a8a8a" }
          }

          Gradient {
              id: normalGradient
              GradientStop { position: 0.0; color: "#8a8a8a" }
              GradientStop { position: 1.0; color: "#686868" }
          }
	  }
    }

    Text {
        color: "white"
        text: parent.label
        style: "Raised";
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        horizontalAlignment: "AlignHCenter";
        verticalAlignment: "AlignVCenter";
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onReleased: { parent.clicked.emit(); }
    }

    states: [
        State {
            name: "released"
            when: !mouseArea.pressed

            PropertyChanges {
                target: buttonGradientRectangle
                gradient: normalGradient
            }
        },
        State {
            name: "pressed"
            when: mouseArea.pressed

            PropertyChanges {
                target: buttonGradientRectangle
                gradient: pressedGradient
            }
        }
    ]
}
