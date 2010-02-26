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

        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: "Fill Mode"
                }

                ComboBox {
                    baseStateFlag: isBaseState
                    items : { ["Stretch", "PreserveAspectFit", "PreserveAspectCrop", "Tile", "TileVertically", "TileHorizontally"] }
                    currentText: backendValues.fillMode.value;
                    onItemsChanged: {
                        currentText =  backendValues.fillMode.value;
                    }
                    backendValue: backendValues.fillMode
                }
            }
        }

        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: "Antialiasing:"
                }

                CheckBox {
                    text: "Smooth";
                    backendValue: backendValues.smooth
                    baseStateFlag: isBaseState;
                    checkable: true;
                    x: 28;  // indent a bit
                }
            }
        }

    }
}
