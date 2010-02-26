import Qt 4.6
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

        Switches {
        }

        ExtendedSwitches {
            id: extendedSwitches;
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
                    Type {
                    }


                    Geometry {
                    }


                    /*Modifiers {

                    }*/


                    WidgetLoader {
                        id: specificsOne;
                        source: specificsUrl;
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
