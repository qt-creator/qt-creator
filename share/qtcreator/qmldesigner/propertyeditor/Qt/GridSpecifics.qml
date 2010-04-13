import Qt 4.7
import Bauhaus 1.0

QWidget {
    layout: QVBoxLayout {
        topMargin: 0
        bottomMargin: 0
        leftMargin: 0
        rightMargin: 0
        spacing: 0
GroupBox {


GroupBox {
    maximumHeight: 200;

    finished: finishedNotify;
    caption: qsTr("Grid")
    id: gridSpecifics;

    layout: QVBoxLayout {

        topMargin: 18;
        bottomMargin: 2;
        leftMargin: 8;
        rightMargin: 8;


            IntEditor {
                id: spacing;
                backendValue: backendValues.spacing;
                caption: qsTr("Spacing")
                baseStateFlag: isBaseState;
                step: 1;
                minimumValue: 0;
                maximumValue: 200;
            }

            IntEditor {
                id: rows;
                backendValue: backendValues.rows;
                caption: qsTr("Rows")
                baseStateFlag: isBaseState;
                step: 1;
                minimumValue: 1;
                maximumValue: 20;
            }

            IntEditor {
                id: columns;
                backendValue: backendValues.columns;
                caption: qsTr("Columns")
                baseStateFlag: isBaseState;
                step: 1;
                minimumValue: 1;
                maximumValue: 20;
            }
    }
}

}
}
