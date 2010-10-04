import Qt 4.7
import Bauhaus 1.0

GroupBox {
    finished: finishedNotify;
    caption: qsTr("Transformation")
    maximumHeight: 200;
    id: transformation;

    layout: VerticalLayout {
        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: qsTr("Origin")
                }
                OriginWidget {
                    id: originWidget
                    
                    contextMenuPolicy: "Qt::ActionsContextMenu"
                    origin: backendValues.transformOrigin.value
                    onOriginChanged: {
                        backendValues.transformOrigin.value = origin;
                    }
                    marked: backendValues.transformOrigin.isInSubState
                    
                    ExtendedFunctionButton {
                        backendValue: backendValues.transformOrigin
                        y: 2
                        x: 56
                        visible: originWidget.enabled
                    }
                    
                    actions:  [
                        QAction { text: qsTr("Top Left"); onTriggered: originWidget.origin = "TopLeft"; },
                        QAction { text: qsTr("Top"); onTriggered: originWidget.origin = "Top"; },
                        QAction { text: qsTr("Top Right"); onTriggered: originWidget.origin = "TopRight"; },
                        QAction { text: qsTr("Left"); onTriggered: originWidget.origin = "Left"; },
                        QAction {text: qsTr("Center"); onTriggered: originWidget.origin = "Center"; },
                        QAction { text: qsTr("Right"); onTriggered: originWidget.origin = "Right"; },
                        QAction { text: qsTr("Bottom Left"); onTriggered: originWidget.origin = "BottomLeft"; },                        
                        QAction { text: qsTr("Bottom"); onTriggered: originWidget.origin = "Bottom"; },
                        QAction { text: qsTr("Bottom Right"); onTriggered: originWidget.origin = "BottomRight"; }
                    ]
                }
            }
        }
        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: qsTr("Scale")
                }

                DoubleSpinBox {
                    text: ""
                    id: scaleSpinBox;

                    backendValue: backendValues.scale;
                    property variant backendValueValue: backendValues.scale.value;
                    minimumWidth: 60;
                    minimum: 0.01
                    maximum: 10
                    singleStep: 0.1
                    baseStateFlag: isBaseState;                    
                }
                SliderWidget {
                    id: scaleSlider;
                    backendValue: backendValues.scale;
                    property variant pureValue: backendValues.scale.value;
                    onPureValueChanged: {
                        if (value != pureValue * 100)
                            value = pureValue * 10;
                    }
                    minimum: 1;
                    maximum: 100;
                    singleStep: 1;
                    onValueChanged: {
                        if ((value > 5) && (value < 100))
                            backendValues.scale.value = value / 10;
                    }
                }
            }
        }
        IntEditor {
            backendValue: backendValues.rotation
            caption: qsTr("Rotation")
            baseStateFlag: isBaseState;
            step: 10;
            minimumValue: -360;
            maximumValue: 360;
        }

        IntEditor {
            backendValue: backendValues.z
            caption: "z"
            baseStateFlag: isBaseState;
            step: 1;
            minimumValue: -100;
            maximumValue: 100;
        }
    }
}
