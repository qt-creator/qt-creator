import Qt 4.6
import Bauhaus 1.0

QScrollArea {
    widgetResizable: true;
    styleSheetFile: ":/qmldesigner/scrollbar.css";
    horizontalScrollBarPolicy: "Qt::ScrollBarAlwaysOff";
    id: ExtendedPane;
    visible: false;
    visible: false;
    content: ProperyEditorExtended;
    QFrame {
        minimumHeight: 440;
        id: ProperyEditorExtended
        layout: QVBoxLayout {
            topMargin: 2;
            bottomMargin: 2;
            leftMargin: 2;
            rightMargin: 2;
            Type {
            }
            Extended {
                id: extendedBox;
            }

            QScrollArea {
            }
        }
    }
}
