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
            caption: qsTr("Path View")
            layout: VerticalLayout {
                QWidget {  // 1
                    layout: HorizontalLayout {

                        Label {
                            text: qsTr("Drag Margin")
                        }

                        DoubleSpinBox {
                            alignRight: false
                            spacing: 4
                            singleStep: 1;
                            backendValue: backendValues.dragMargin
                            minimum: -100;
                            maximum: 100;
                            baseStateFlag: isBaseState;
                        }
                    }
                }
                QWidget {  // 1
                    layout: HorizontalLayout {

                        Label {
                            text: qsTr("Flick Deceleration")
                        }

                        DoubleSpinBox {
                            alignRight: false
                            spacing: 4
                            singleStep: 1;
                            backendValue: backendValues.flickDeceleration
                            minimum: 0;
                            maximum: 1000;
                            baseStateFlag: isBaseState;
                        }
                    }
                }
                QWidget {  // 1
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Follows Current")
                            toolTip: qsTr("A user cannot drag or flick a PathView that is not interactive.")
                        }
                        CheckBox {
                            backendValue: backendValues.interactive
                            baseStateFlag: isBaseState;
                            checkable: True
                        }
                    }
                }
                QWidget {  // 1
                    layout: HorizontalLayout {

                        Label {
                            text: qsTr("Offset")
                            toolTip: qsTr("Specifies how far along the path the items are from their initial positions. This is a real number that ranges from 0.0 to the count of items in the model.")
                        }
                        DoubleSpinBox {
                            alignRight: false
                            spacing: 4
                            singleStep: 1;
                            backendValue: backendValues.offset
                            minimum: 0;
                            maximum: 100;
                            baseStateFlag: isBaseState;
                        }
                    }
                }
                IntEditor {
                    backendValue: backendValues.pathItemCount
                    caption: qsTr("Item Count")
                    toolTip: qsTr("pathItemCount: number of items visible on the path at any one time.")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: -1;
                    maximumValue: 1000;
                }
            }
        }
        GroupBox {
            finished: finishedNotify;
            caption: qsTr("Grid View ViewHighlight")
            layout: VerticalLayout {
                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Highlight Range")
                        }

                        ComboBox {
                            baseStateFlag: isBaseState
                            items : { ["NoHighlightRange", "ApplyRange", "StrictlyEnforceRange"] }
                            currentText: backendValues.highlightRangeMode.value;
                            onItemsChanged: {
                                currentText =  backendValues.highlightRangeMode.value;
                            }
                            backendValue: backendValues.highlightRangeMode
                        }
                    }
                } //QWidget
                IntEditor {
                    backendValue: backendValues.highlightMoveDuration
                    caption: qsTr("Move Duration")
                    toolTip: qsTr("Move animation duration of the highlight delegate.")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 1000;
                }
                IntEditor {
                    backendValue: backendValues.preferredHighlightBegin
                    caption: qsTr("Preferred Begin")
                    toolTip: qsTr("Preferred highlight begin - must be smaller than Preferred End.")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 1000;
                }
                IntEditor {
                    backendValue: backendValues.preferredHighlightEnd
                    caption: qsTr("Preferred End")
                    toolTip: qsTr("Preferred highlight end - must be larger than Preferred Begin.")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 1000;
                }
                QWidget {  // 1
                    layout: HorizontalLayout {

                        Label {
                            text: qsTr("Follows Current")
                        }
                        CheckBox {
                            backendValue: backendValues.highlightFollowsCurrentItem
                            toolTip: qsTr("Determines whether the highlight is managed by the view.")
                            baseStateFlag: isBaseState;
                            checkable: True
                        }
                    }
                }
            }
        }
        QScrollArea {
        }
    }
}
