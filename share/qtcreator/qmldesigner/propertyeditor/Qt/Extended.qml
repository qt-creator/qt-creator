import Qt 4.6
import Bauhaus 1.0

GroupBox {
    caption: "Effect"
    id: extended;
    maximumHeight: 260;

    layout: VerticalLayout{

        property var effect: backendValues.effect
        property var complexNode: effect.complexNode

        QWidget  {
            maximumHeight: 40;
            layout: HorizontalLayout {
                Label {
                    text: "Effect ";
                }
                QComboBox {
                    enabled: isBaseState;
                    property var type: backendValues.effect.complexNode.type
                    property var dirty;
                    id: effectComboBox;
                    items : { [
                            "None",
                            "Blur",
                            "Opacity",
                            "Colorize",
                            "DropShadow"
                            ] }

                            onCurrentTextChanged: {

                                if (dirty) //avoid recursion;
                                return;
                                if (currentText == "")

                                if (backendValues.effect.complexNode.exists)
                                backendValues.effect.complexNode.remove();
                                if (currentText == "None") {
                                    ;
                                    } else if (backendValues.effect.complexNode != null) {
                                    backendValues.effect.complexNode.add("Qt/" + currentText);
                                    }
                            }

                            onTypeChanged: {
                                dirty = true;
                                if (backendValues.effect.complexNode.exists)
                                currentText = backendValues.effect.complexNode.type;
                                else
                                currentText = "None";
                                dirty = false;
                            }
                }
                QWidget {
                    fixedWidth: 100
                }
            }
            }// QWidget

            property var properties: complexNode == null ? null : complexNode.properties

            QWidget {
                minimumHeight: 20;
                layout: QVBoxLayout {

                    QWidget {
                        visible: effectComboBox.currentText == "Blur";
                        layout: QVBoxLayout {
                            topMargin: 12;
                            IntEditor {
                                id: blurRadius;
                                backendValue: backendValues.effect.complexNode.exists ? backendValues.effect.complexNode.properties.blurRadius : null;
                                caption: "Blur Radius:"
                                baseStateFlag: isBaseState;

                                step: 1;
                                minimumValue: 0;
                                maximumValue: 20;
                            }
                        }
                    }

                    QWidget {
                        visible: effectComboBox.currentText == "Opacity";
                        layout: QVBoxLayout {
                            DoubleSpinBox {
                                id: opcacityEffectSpinBox;
                                backendValue: backendValues.effect.complexNode.exists ? backendValues.effect.complexNode.properties.opacity : null;
                                minimum: 0;
                                maximum: 1;
                                singleStep: 0.1;
                                baseStateFlag: isBaseState;
                            }
                        }
                    }

                    QWidget {
                        visible: effectComboBox.currentText == "Colorize";
                        layout: QVBoxLayout {

                            property var colorProp: properties == null ? null : properties.color


                            ColorLabel {
                                text: "    Color"
                            }

                            ColorGroupBox {

                                finished: finishedNotify

                                backendColor: properties.color
                            }

                        }
                    }

                    QWidget {
                        visible: effectComboBox.currentText == "Pixelize";
                        layout: QVBoxLayout {
                            topMargin: 12;
                            IntEditor {
                                id: pixelSize;
                                backendValue: backendValues.effect.complexNode.exists ? backendValues.effect.complexNode.properties.pixelSize : null;
                                caption: "Pixel Size:"
                                baseStateFlag: isBaseState;

                                step: 1;
                                minimumValue: 0;
                                maximumValue: 20;
                            }
                        }
                    }

                    QWidget {
                        visible: effectComboBox.currentText == "DropShadow";
                        layout: QVBoxLayout {

                            topMargin: 12;
                            IntEditor {
                                id: blurRadiusShadow;
                                backendValue: backendValues.effect.complexNode.exists ? backendValues.effect.complexNode.properties.blurRadius : null;
                                caption: "Blur Radius:"
                                baseStateFlag: isBaseState;

                                step: 1;
                                minimumValue: 0;
                                maximumValue: 20;
                            }


                            ColorLabel {
                                text: "    Color"
                            }

                            ColorGroupBox {

                                finished: finishedNotify

                                backendColor: properties.color
                            }

                            IntEditor {
                                id: xOffset;
                                backendValue: backendValues.effect.complexNode.exists ? backendValues.effect.complexNode.properties.xOffset : null;
                                caption: "x Offset:     "
                                baseStateFlag: isBaseState;

                                step: 1;
                                minimumValue: 0;
                                maximumValue: 20;
                            }

                            IntEditor {
                                id: yOffset;
                                backendValue: backendValues.effect.complexNode.exists ? backendValues.effect.complexNode.properties.yOffset : null;
                                caption: "y Offset:     "
                                baseStateFlag: isBaseState;

                                step: 1;
                                minimumValue: 0;
                                maximumValue: 20;
                            }
                        }
                    }
                }
            }
            } //QVBoxLayout
            } //GroupBox
