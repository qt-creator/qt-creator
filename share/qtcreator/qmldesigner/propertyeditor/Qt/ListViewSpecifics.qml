import Qt 4.6
import Bauhaus 1.0

GroupBox {
 finished: finishedNotify;
    caption: "List View";

    layout: QVBoxLayout {
        topMargin: 15;
        bottomMargin: 6;
        leftMargin: 0;
        rightMargin: 0;

        QWidget {
            id: contentWidget;
             maximumHeight: 260;

             layout: QHBoxLayout {
                topMargin: 0;
                bottomMargin: 0;
                leftMargin: 10;
                rightMargin: 10;

                QWidget {
                    layout: QVBoxLayout {
                        topMargin: 0;
                        bottomMargin: 0;
                        leftMargin: 0;
                        rightMargin: 0;
                        QLabel {
                            minimumHeight: 22;
                            text: "Highlight Follows:"
                            font.bold: true;
                        }

                        QLabel {
                            minimumHeight: 22;
                            text: "Key Navigation Wraps:"
                            font.bold: true;
                        }

                        QLabel {
                            minimumHeight: 22;
                            text: "Snap Position:"
                            font.bold: true;
                        }

                        QLabel {
                            minimumHeight: 22;
                            text: "Spacing:"
                            font.bold: true;
                        }
                    }
                }

                QWidget {
                    layout: QVBoxLayout {
                        topMargin: 0;
                        bottomMargin: 0;
                        leftMargin: 0;
                        rightMargin: 0;

                        CheckBox {
                            id: highlightFollowsCurrentItemCheckBox;
                            text: "";
                            backendValue: backendValues.highlightFollowsCurrentItem;
                            baseStateFlag: isBaseState;
                            checkable: true;
                        }

                        CheckBox {
                            id: wrapCheckBox;
                            text: "";
                            backendValue: backendValues.wrap;
                            baseStateFlag: isBaseState;
                            checkable: true;
                        }

                        SpinBox {
                            id: snapPositionSpinBox;
                            objectName: "snapPositionSpinBox";
                            backendValue: backendValues.snapPosition;
                            minimumWidth: 30;
                            minimum: 0;
                            maximum: 1000;
                            singleStep: 1;
                            baseStateFlag: isBaseState;
                        }

                        SpinBox {
                            id: spacingSpinBox;
                            objectName: "spacingSpinBox";
                            backendValue: backendValues.spacing;
                            minimumWidth: 30;
                            minimum: 0;
                            maximum: 1000;
                            singleStep: 1;
                            baseStateFlag: isBaseState;
                        }

                    }
                }
            }
        }
    }
}
