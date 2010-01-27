import Qt 4.6
import Bauhaus 1.0

GroupBox {

    finished: finishedNotify;
    caption: "Rect"

    maximumHeight: 340;

    layout: VerticalLayout {

        IntEditor {
            backendValue: backendValues.radius
            caption: "Radius"
            baseStateFlag: isBaseState;
            step: 1;
            minimumValue: 0;
            maximumValue: 100;
        }        


        IntEditor {
            id: borderWidth;
            backendValue: backendValues.border_width === undefined ? 0 : backendValues.border_width

            caption: "Pen Width"
            baseStateFlag: isBaseState;

            step: 1;
            minimumValue: 0;
            maximumValue: 100;
        }

        ColorWidget {
            text: "Color";
            color: backendValues.color === undefined ? null : backendValues.color.value;
            onColorChanged: {
                backendValues.color.value = strColor;
            }
            //modelNode: backendValues.color.modelNode;
            complexGradientNode: backendValues.color === undefined ? null : backendValues.color.complexNode

            //showGradientButton: true;
        }

        ColorWidget {
            text: "Tint color";
            color: backendValues.tintColor === undefined ? "black" : backendValues.tintColor.value
            onColorChanged: {
                backendValues.color.value = strColor;
            }
        }

        ColorWidget {
            id: PenColor;
            text: "Pen Color";
            minimumHeight: 20;
            minimumWidth: 20;
            color: backendValues.border_color.value;
            onColorChanged: {
                backendValues.border_color.value = strColor;
            }
        }

    }
}
