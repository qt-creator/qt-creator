import Qt 4.6
import Bauhaus 1.0

GroupBox {
    finished: finishedNotify;
    caption: "Flickable";

    layout: QVBoxLayout {
	    topMargin: 15;
        bottomMargin: 6;
        leftMargin: 0;
        rightMargin: 0;

		QWidget {
			id: contentWidget;
			 maximumHeight: 220;

			 layout: QHBoxLayout {
				topMargin: 0;
				bottomMargin: 0;
				leftMargin: 10;
				rightMargin: 10;

				QWidget {
					layout: QVBoxLayout {
						topMargin: 0;
						bottomMargin: 0;
						leftMargin: 0;
						rightMargin: 0;
						QLabel {
						    minimumHeight: 20;
                            text: "Horizontal Velocity:"
                            font.bold: true;
						}

						QLabel {
						    minimumHeight: 20;
                            text: "Vertical Velocity:"
                            font.bold: true;
						}

						QLabel {
						    minimumHeight: 20;
                            text: "Maximum Flick Velocity:"
                            font.bold: true;
						}

						QLabel {
						    minimumHeight: 20;
                            text: "Over Shoot:"
                            font.bold: true;
						}
					}
				}


				QWidget {
					layout: QVBoxLayout {
						topMargin: 0;
						bottomMargin: 0;
						leftMargin: 0;
						rightMargin: 0;


						DoubleSpinBox {
							id: HorizontalVelocitySpinBox;
							objectName: "HorizontalVelocitySpinBox";
							backendValue: backendValues.horizontalVelocity;
							minimumWidth: 30;
							minimum: 0.1
							maximum: 10
							singleStep: 0.1
							baseStateFlag: isBaseState;
						}

						DoubleSpinBox {
							id: VerticalVelocitySpinBox;
							objectName: "VerticalVelocitySpinBox";
							backendValue: backendValues.verticalVelocity;
							minimumWidth: 30;
							minimum: 0.1
							maximum: 10
							singleStep: 0.1
							baseStateFlag: isBaseState;
						}

						DoubleSpinBox {
							id: MaximumVelocitySpinBox;
							objectName: "MaximumVelocitySpinBox";
							backendValue: backendValues.maximumFlickVelocity;
							minimumWidth: 30;
							minimum: 0.1
							maximum: 10
							singleStep: 0.1
							baseStateFlag: isBaseState;
						}

						CheckBox {
							id: OvershootCheckBox;
							text: "overshoot";
							backendValue: backendValues.overShoot;
							baseStateFlag: isBaseState;
							checkable: true;
						}
					}
				}

			}
		}
    }
}
