import Qt 4.6
import Bauhaus 1.0

GroupBox {
    maximumHeight: 240;

    finished: finishedNotify;
    caption: "Image";

    layout: VerticalLayout {

        FileWidget {
            enabled: isBaseState || backendValues.id.value != "";
            text: "Source: ";
            fileName: backendValues.source.value;
            onFileNameChanged: {
                backendValues.source.value = fileName;
            }
        }
    }
}
