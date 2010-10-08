import Qt 4.7
import Bauhaus 1.0

PropertyFrame {
    id: frame;

    ExpressionEditor {
        id: expressionEdit
    }
    layout: QVBoxLayout {
        topMargin: 0;
        bottomMargin: 0;
        leftMargin: 0;
        rightMargin: 0;
        spacing: 0;

        Type {
        }

        Geometry {
        }

        Visibility {

        }

        HorizontalWhiteLine {
            maximumHeight: 4;
            styleSheet: "QLineEdit {border: 2px solid #707070; min-height: 0px; max-height: 0px;}";
        }

        Switches {
        }

        HorizontalWhiteLine {
        }

        ScrollArea {
            styleSheetFile: ":/qmldesigner/scrollbar.css";
            widgetResizable: true;

            finished: finishedNotify;

            horizontalScrollBarPolicy: "Qt::ScrollBarAlwaysOff";
            id: standardPane;
            content: properyEditorStandard;
            QFrame {
                //minimumHeight: 1100
                id: properyEditorStandard
                layout: QVBoxLayout {
                    topMargin: 0;
                    bottomMargin: 0;
                    leftMargin: 0;
                    rightMargin: 0;
                    spacing: 0;

                    WidgetLoader {
                        id: specificsOne;
                        source: specificsUrl;
                    }

                    WidgetLoader {
                        id: specificsTwo;
                        baseUrl: globalBaseUrl;
                        qmlData: specificQmlData;
                    }
                    QScrollArea {
                    }
                } // layout
            } //QWidget
        } //QScrollArea
        ExtendedPane {
            id: extendedPane;
        }
        LayoutPane {
            id: layoutPane;
        }
    }
}
