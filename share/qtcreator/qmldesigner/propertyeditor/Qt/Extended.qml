import Qt 4.6
import Bauhaus 1.0

GroupBox {
caption: "Effects"
id: Extended;
maximumHeight: 260;

layout: QVBoxLayout {
    topMargin: 12;
    leftMargin: 10;
    rightMargin: 10;

    property var effect: backendValues == null || backendValues.effect === undefined ? null : backendValues.effect
    property var complexNode : effect === null ? null : effect.complexNode

    QWidget  {
        maximumHeight: 40;
        layout: QHBoxLayout {
            QLabel {
                text: "Effects: ";
                font.bold: true;
             }
             QComboBox {
                 enabled: isBaseState;
                 property var type: complexNode == null ? null : complexNode.type
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
                      if (complexNode != null && complexNode.exists)
                             complexNode.remove();
                     if (currentText == "None") {
                         ;
                     } else if (complexNode != null) {
                         complexNode.add("Qt/" + currentText);
                     }
                 }

                 onTypeChanged: {
                     dirty = true;
                     if (complexNode != null && complexNode.type != "")
                         currentText = complexNode.type;
                     else
                         currentText = "None";
                     dirty = false;
                 }
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
                backendValue: properties == null ? null : properties.blurRadius;
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
                                  id: OpcacityEffectSpinBox;
                                  objectName: "OpcacityEffectSpinBox";
                                  backendValue: properties == null ? null : properties.opacity;
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

          ColorWidget {
                id: ColorizeColor;
                text: "Color:";
                minimumHeight: 20;
                minimumWidth: 20;
                color: (colorProp == null) ? "black" : colorProp.value;
                onColorChanged: {
                    colorProp.value = strColor;
                }
            }

         }
      }

      QWidget {
          visible: effectComboBox.currentText == "Pixelize";
          layout: QVBoxLayout {
              topMargin: 12;
              IntEditor {
                id: pixelSize;
                backendValue: properties == null ? null : properties.pixelSize;
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
                backendValue: properties == null ? null : properties.blurRadius;
                caption: "Blur Radius:"
                baseStateFlag: isBaseState;

                step: 1;
                minimumValue: 0;
                maximumValue: 20;
            }

            ColorWidget {
                id: DropShadowColor;
                maximumHeight: 40;
                text: "Color:";
                minimumHeight: 20;
                minimumWidth: 20;
                color: properties == null || properties.color == null ? "black" : properties.color.value
                onColorChanged: {
                    if (properties != null && properties.color != null)
                        properties.color.value = strColor;
                }
            }

              IntEditor {
                id: xOffset;
                backendValue: properties == null ? 0 : properties.xOffset;
                caption: "x Offset:     "
                baseStateFlag: isBaseState;

                step: 1;
                minimumValue: 0;
                maximumValue: 20;
            }

            IntEditor {
                id: yOffset;
                backendValue: properties == null ? 0 : properties.yOffset;
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
