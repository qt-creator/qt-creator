import Qt 4.6

GroupBox {
  id: Type;
  finished: finishedNotify;
  caption: "Type";

  maximumHeight: 100;
  minimumWidth: 280;
  layout: QHBoxLayout {

            topMargin: 2;
            bottomMargin: 0;
            leftMargin: 10;
            rightMargin: 10;



            QWidget {
            layout: QVBoxLayout {
              topMargin: 6;
              bottomMargin: 0;
              leftMargin: 10;
              bottomMargin: 10;

            QLabel {
            text: "Type:";
            windowTextColor: isBaseState ? "#000000" : "#FF0000";
            }

            QLabel {
            font.bold: true;
            text: "Id:";
            }

             QLabel {
               text: "state:";
               font.bold: true;
            }

            } //QVBoxLayout
            } //QWidget
            QWidget {

            layout: QVBoxLayout {
              topMargin: 6;
              bottomMargin: 0;
              leftMargin: 10;
              bottomMargin: 10;

              QLineEdit {
              styleSheetFile: "typeLabel.css"
              text: backendValues.className === undefined ? "" : backendValues.className.value;
              readOnly :true;
            }

            QLineEdit {
              id: IdLineEdit;
              objectName: "IdLineEdit";
              readOnly: isBaseState != true;
              text: backendValues.id === undefined ? "" : backendValues.id.value;
              onEditingFinished: {
              backendValues.id.value = text;
              }
            } //LineEdit

             QLineEdit {
               visible: isBaseState != true;
               textColor: "#ff0000";
               readOnly :true;
               text: stateName;
               styleSheetFile: "typeLabel.css"
                onTextChanged: {
                  if (!visible) {
                      IdLineEdit.textColor = "#000000";
                  } else {
                      IdLineEdit.textColor = "#777777";
                  }
                  }
            } //LineEdit

             QLineEdit {
               visible: isBaseState;
               readOnly :true;
               text: "";
               styleSheetFile: "typeLabel.css"
            } //LineEdit

            } //QVBoxLayout
            } //QWidget


            } //QHBoxLayout
}
