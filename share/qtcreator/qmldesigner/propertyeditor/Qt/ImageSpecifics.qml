import Qt 4.6
import Bauhaus 1.0

GroupBox {
    maximumHeight: 240;

    finished: finishedNotify;
    caption: "Image";

    layout: QVBoxLayout {
        topMargin: 12;
        bottomMargin: 12;
        leftMargin: 2;
        rightMargin: 2;

        FileWidget {
            enabled: isBaseState || backendValues.id.value != "";
            maximumWidth: 250;
            text: "Source: ";
            fileName: backendValues.source.value;
            onFileNameChanged: {
                backendValues.source.value = fileName;
            }
        }
    }
}
