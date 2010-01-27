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
            topMargin: 0;
            bottomMargin: 0;
            leftMargin: 0;
            rightMargin: 0;
			spacing: 0;
            Modifiers {
            }
            Extended {
                id: extendedBox;
            }

            QScrollArea {
            }
        }
    }
}
