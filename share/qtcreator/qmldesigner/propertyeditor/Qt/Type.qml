import Qt 4.6

GroupBox {
    id: Type;
    finished: finishedNotify;
    caption: "Type";

    layout: VerticalLayout {
        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: "Type";
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
                    id: IdLineEdit;
                    objectName: "IdLineEdit";
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
