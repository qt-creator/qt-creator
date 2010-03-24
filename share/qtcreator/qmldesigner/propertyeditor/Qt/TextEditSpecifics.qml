import Qt 4.6
import Bauhaus 1.0

QWidget {
    id: textSpecifics;

    layout: QVBoxLayout {
        topMargin: 0
        bottomMargin: 0
        leftMargin: 0
        rightMargin: 0
        spacing: 0

        StandardTextGroupBox {
            finished: finishedNotify;
        }

        StandardTextColorGroupBox {
            finished: finishedNotify;
			showSelectionColor: true;
			showSelectedTextColor: true;
        }

        GroupBox {
            caption: "Text Edit"
            finished: finishedNotify;
            layout: VerticalLayout {

                QWidget {
                    layout: HorizontalLayout {
					rightMargin: 12
                        Label {
                            text: "Format"
                        }
                        ComboBox {
                            baseStateFlag: isBaseState
                            minimumHeight: 22;
                            items : { ["PlainText", "RichText", "AutoText"] }
                            currentText: backendValues.textFormat.value;
                            onItemsChanged: {
                                currentText =  backendValues.textFormat.value;
                            }

                            backendValue: backendValues.textFormat
                        }
                    }
                }

            }
        }

        FontGroupBox {
            finished: finishedNotify;

        }

        TextInputGroupBox {
            finished: finishedNotify;
        }
        QScrollArea {
        }
    }
}
