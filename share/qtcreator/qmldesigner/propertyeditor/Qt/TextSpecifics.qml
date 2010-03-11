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


        StandardTextColorGroupBox {
            showStyleColor: true
            finished: finishedNotify;
        }
		
		    StandardTextGroupBox {
            showIsWrapping: true
            showVerticalAlignment: true
            finished: finishedNotify;
        }

        FontGroupBox {
            finished: finishedNotify;
            showStyle: true
        }

        QScrollArea {
        }
    }
}
