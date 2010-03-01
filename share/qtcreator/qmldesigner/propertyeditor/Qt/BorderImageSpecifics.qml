import Qt 4.6
import Bauhaus 1.0

QWidget {
    layout: QVBoxLayout {
        topMargin: 0
        bottomMargin: 0
        leftMargin: 0
        rightMargin: 0
        spacing: 0


        GroupBox {
            maximumHeight: 240;

            finished: finishedNotify;
            caption: "Image";

            layout: VerticalLayout {

                FileWidget {
                    enabled: isBaseState || backendValues.id.value != "";
                    maximumWidth: 250;
                    text: "Source";
                    fileName: backendValues.source.value;
                    onFileNameChanged: {
                        backendValues.source.value = fileName;
                    }
                }


                IntEditor {
                    id: pixelSize;
                    backendValue: backendValues.border_left;
                    caption: "Left"
                    baseStateFlag: isBaseState;

                    step: 1;
                    minimumValue: 0;
                    maximumValue: 2000;
                }

                IntEditor {
                    backendValue: backendValues.border_right;
                    caption: "Right"
                    baseStateFlag: isBaseState;

                    step: 1;
                    minimumValue: 0;
                    maximumValue: 2000;
                }

                IntEditor {
                    backendValue: backendValues.border_top;
                    caption: "Top"
                    baseStateFlag: isBaseState;

                    step: 1;
                    minimumValue: 0;
                    maximumValue: 2000;
                }

                IntEditor {
                    backendValue: backendValues.border_bottom;
                    caption: "Bottom"
                    baseStateFlag: isBaseState;

                    step: 1;
                    minimumValue: 0;
                    maximumValue: 2000;
                }
            }
        }

    }
}
