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
            finished: finishedNotify;
            caption: qsTr("Flow")
            layout: VerticalLayout {
                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Flow")
                        }

                        ComboBox {
                            baseStateFlag: isBaseState
                            items : { ["LeftToRight", "TopToBottom"] }
                            currentText: backendValues.flow.value;
                            onItemsChanged: {
                                currentText =  backendValues.flow.value;
                            }
                            backendValue: backendValues.flow
                        }
                    }
                } //QWidget
//                Qt namespace enums not supported by the rewriter
//                QWidget {
//                    layout: HorizontalLayout {
//                        Label {
//                            text: qsTr("Layout direction")
//                        }

//                        ComboBox {
//                            baseStateFlag: isBaseState
//                            items : { ["LeftToRight", "RightToLeft"] }
//                            currentText: backendValues.layoutDirection.value;
//                            onItemsChanged: {
//                                currentText =  backendValues.layoutDirection.value;
//                            }
//                            backendValue: backendValues.layoutDirection
//                        }
//                    }
//                } //QWidget
                IntEditor {
                    backendValue: backendValues.spacing
                    caption: qsTr("Spacing")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 100;
                }
            }
        }
    }
}
