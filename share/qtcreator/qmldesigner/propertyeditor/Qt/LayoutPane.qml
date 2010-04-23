import Qt 4.7
import Bauhaus 1.0

QScrollArea {
    widgetResizable: true;
    styleSheetFile: ":/qmldesigner/scrollbar.css";
    horizontalScrollBarPolicy: "Qt::ScrollBarAlwaysOff";
    id: layoutPane;
    visible: false;
    content: properyEditorLayout;
    QFrame {
        id: properyEditorLayout;
        layout: QVBoxLayout {
            topMargin: 0;
            bottomMargin: 0;
            leftMargin: 0;
            rightMargin: 0;
            spacing: 0

            Geometry {
            }

            Layout {
                id: layoutBox;
            }
            QScrollArea {
            }
        }
    }
}
