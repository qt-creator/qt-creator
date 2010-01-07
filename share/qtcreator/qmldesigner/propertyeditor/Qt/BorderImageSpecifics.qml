import Qt 4.6
import Bauhaus 1.0

GroupBox {
    maximumHeight: 240;


    finished: finishedNotify;
    caption: "Image";

    layout: QVBoxLayout {
        topMargin: 12;
        bottomMargin: 12;
        leftMargin: 12;
        rightMargin: 12;

        FileWidget {
            enabled: isBaseState || backendValues.id.value != "";
            maximumWidth: 250;
            text: "Source: ";
            fileName: backendValues.source.value;
            onFileNameChanged: {
                backendValues.source.value = fileName;
            }
        }


        IntEditor {
            id: pixelSize;
            backendValue: backendValues.border_left;
            caption: "Border Left:       "
            baseStateFlag: isBaseState;

            step: 1;
            minimumValue: 0;
            maximumValue: 2000;
        }

        IntEditor {
            backendValue: backendValues.border_right;
            caption: "Border Right:    "
            baseStateFlag: isBaseState;

            step: 1;
            minimumValue: 0;
            maximumValue: 2000;
            }

            IntEditor {
                backendValue: backendValues.border_top;
                caption: "Border Top:       "
                baseStateFlag: isBaseState;

                step: 1;
                minimumValue: 0;
                maximumValue: 2000;
            }

            IntEditor {
                backendValue: backendValues.border_bottom;
                caption: "Border Bottom:"
                baseStateFlag: isBaseState;

                step: 1;
                minimumValue: 0;
                maximumValue: 2000;
            }
    }
}
