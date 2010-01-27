import Qt 4.6

GroupBox {
    id: Type;
    finished: finishedNotify;
    caption: "Type";

    maximumHeight: 100;
    minimumWidth: 280;
    layout: QHBoxLayout {
        topMargin: 18;
        bottomMargin: 0;
        leftMargin: 60;
        rightMargin: 10;



        QWidget {
            layout: QVBoxLayout {
                topMargin: 6;
                bottomMargin: 0;
                leftMargin: 10;
                bottomMargin: 10;

                QLabel {
                    text: "Type";
                    windowTextColor: isBaseState ? "#000000" : "#FF0000";
                    alignment: "Qt::AlignRight | Qt::AlignVCenter"
                }

                QLabel {
                    font.bold: true;
                    text: "Id";
                    alignment: "Qt::AlignRight | Qt::AlignVCenter"
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


            } //QVBoxLayout
        } //QWidget


    } //QHBoxLayout
}
