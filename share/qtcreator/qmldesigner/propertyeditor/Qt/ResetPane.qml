import Qt 4.6
import Bauhaus 1.0

QScrollArea {
    horizontalScrollBarPolicy: "Qt::ScrollBarAlwaysOff";
    id: resetPane;
    visible: false;
    content: properyEditorReset;
    QFrame {
    //minimumHeight: 1800;
    id: properyEditorReset
    layout: QVBoxLayout {
    topMargin: 2;
    bottomMargin: 2;
    leftMargin: 2;
    rightMargin: 2;

    Type {
    }
    ResetWidget {
        id: resetWidget;
        //minimumHeight: 2000;


        height: 500;
        width: 200;
        backendObject: backendValues;

        QLineEdit {
            visible: false;
            text: backendValues.id;
            onTextChanged: {
                resetWidget.resetView();
            }
        }
    }


    }
    }
}
