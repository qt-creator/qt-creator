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

        HorizontalWhiteLine {
        }

        ScrollArea {
            styleSheetFile: ":/qmldesigner/scrollbar.css";
            widgetResizable: true;
            content: QFrame {
                maximumHeight: 38;
                layout: QVBoxLayout {
                    topMargin: 0;
                    bottomMargin: 0;
                    leftMargin: 0;
                    rightMargin: 0;
                    QExtGroupBox {
                        font.bold: true;
                        maximumHeight: 100;
                        minimumWidth: 280;
                        minimumHeight: 32;
                        layout: QHBoxLayout {
                            topMargin: 6;
                            bottomMargin: 2;
                            QLabel {
                                minimumHeight: 20;
                                text: "no or multiple items selected";
                                alignment: "AlignHCenter";
                            }
                        }
                    }
                }
            }
        }
    }
}
