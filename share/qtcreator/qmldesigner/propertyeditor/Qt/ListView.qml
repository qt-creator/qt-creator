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

        QScrollArea {
            horizontalScrollBarPolicy: "Qt::ScrollBarAlwaysOff";
            id: standardPane;
            content: ProperyEditorStandard;
            QFrame {
                minimumHeight: 400;
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

                    QScrollArea {
                    }

                    } // layout
                    } //QWidget
                    } //QScrollArea


                    QScrollArea {
                        horizontalScrollBarPolicy: "Qt::ScrollBarAlwaysOff";
                        id: specialPane;
                        visible: false;
                        visible: false;
                        content: ProperyEditorSpecial;
                        QFrame {
                            minimumHeight: 200;
                            id: ProperyEditorSpecial
                            layout: QVBoxLayout {
                                topMargin: 2;
                                bottomMargin: 2;
                                leftMargin: 2;
                                rightMargin: 2;
                                Type {
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


                    ResetPane {
                        id: resetPane;
                    }


    }
}
