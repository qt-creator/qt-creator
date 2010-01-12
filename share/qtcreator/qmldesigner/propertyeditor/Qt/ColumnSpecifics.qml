import Qt 4.6
import Bauhaus 1.0

GroupBox {
    maximumHeight: 200;

    finished: finishedNotify;
    caption: "Grid";
    id: GridSpecifics;

    layout: QVBoxLayout {

        topMargin: 18;
        bottomMargin: 2;
        leftMargin: 8;
        rightMargin: 8;


            IntEditor {
                id: spacing;
                backendValue: backendValues.spacing;
                caption: "Spacing:   "
                baseStateFlag: isBaseState;
                step: 1;
                minimumValue: 0;
                maximumValue: 2000;
            }
    }
}
