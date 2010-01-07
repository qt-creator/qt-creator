import Qt 4.6
import Bauhaus 1.0

PropertyFrame {
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

        ScrollArea {
            styleSheetFile: ":/qmldesigner/scrollbar.css";
            widgetResizable: true;

            finished: finishedNotify;

            horizontalScrollBarPolicy: "Qt::ScrollBarAlwaysOff";
            id: standardPane;


            content: ProperyEditorStandard;
            QFrame {
                minimumHeight: 1100;
                id: ProperyEditorStandard
                layout: QVBoxLayout {
                    topMargin: 2;
                    bottomMargin: 2;
                    leftMargin: 2;
                    rightMargin: 2;
                    Type {
                    }
                    HorizontalLine {
                    }

                    Geometry {
                    }

                    Modifiers {

                    }

                    WidgetLoader {
                        id: specificsOne;
                        source: specificsUrl;
                    }


                    QScrollArea {
                    }

                    } // layout
                    } //QWidget
                    } //QScrollArea


                    ScrollArea {
                        widgetResizable: true;
                        styleSheetFile: ":/qmldesigner/scrollbar.css";
                        finished: finishedNotify;
                        horizontalScrollBarPolicy: "Qt::ScrollBarAlwaysOff";
                        id: specialPane;
                        visible: false;
                        visible: false;
                        content: ProperyEditorSpecial;
                        QFrame {
                            minimumHeight: 600;
                            id: ProperyEditorSpecial
                            layout: QVBoxLayout {
                                topMargin: 2;
                                bottomMargin: 2;
                                leftMargin: 2;
                                rightMargin: 2;
                                Type {
                                }

                                WidgetLoader {
                                    id: specificsOne2;
                                    source: specificsUrl;
                                }

                                QScrollArea {
                                }
                            }
                        }
                    }

                    ExtendedPane {
                        id: extendedPane;
                    }

                    LayoutPane {
                        id: layoutPane;
                    }

    }
}
