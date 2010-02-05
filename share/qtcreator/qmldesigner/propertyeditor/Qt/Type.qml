import Qt 4.6
import Bauhaus 1.0

GroupBox {
    id: type;
    finished: finishedNotify;
    caption: "type";

    layout: VerticalLayout {
        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: "type";
                    windowTextColor: isBaseState ? "#000000" : "#FF0000";                 
                }                
                QLineEdit {
                    styleSheetFile: "typeLabel.css"
                    text: backendValues.className === undefined ? "" : backendValues.className.value;
                    readOnly :true;
                }
            }
        }
        QWidget {            
            layout: HorizontalLayout {
                Label {
                    text: "Id";
                }

                QLineEdit {
                    id: idLineEdit;
                    objectName: "idLineEdit";
                    readOnly: isBaseState != true;
                    text: backendValues.id === undefined ? "" : backendValues.id.value;
                    onEditingFinished: {
                        backendValues.id.value = text;
                    }
                }
            }
        }
    }
}
