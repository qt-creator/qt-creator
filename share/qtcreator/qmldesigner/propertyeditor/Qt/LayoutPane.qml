import Qt 4.6
import Bauhaus 1.0

QScrollArea {
    widgetResizable: true;
    styleSheetFile: ":/qmldesigner/scrollbar.css";
    horizontalScrollBarPolicy: "Qt::ScrollBarAlwaysOff";
    id: LayoutPane;
    visible: false;
    content: ProperyEditorLayout;
        QFrame {
            enabled: isBaseState;
            id: ProperyEditorLayout;
            minimumHeight: 460;
            layout: QVBoxLayout {
            topMargin: 2;
            bottomMargin: 2;
            leftMargin: 2;
            rightMargin: 2;
            Type {
            }
            Layout {
                id: layoutBox;
            }
            QScrollArea {
            }
        }
    }
}