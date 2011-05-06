import Qt 4.7
import Bauhaus 1.0

QWidget {
    layout: QVBoxLayout {
        topMargin: 0
        bottomMargin: 0
        leftMargin: 0
        rightMargin: 0
        spacing: 0
        FlickableGroupBox {
            finished: finishedNotify;
        }
        GroupBox {
            finished: finishedNotify;
            caption: qsTr("List View")
            layout: VerticalLayout {
                IntEditor {
                    backendValue: backendValues.cacheBuffer
                    caption: qsTr("Cache")
                    toolTip: qsTr("Cache Buffer")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 1000;
                }
                IntEditor {
                    backendValue: backendValues.cellHeight
                    caption: qsTr("Cell Height")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 200;
                }
                IntEditor {
                    backendValue: backendValues.cellWidth
                    caption: qsTr("Cell Width")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 1000;
                }
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
                IntEditor {
                    backendValue: backendValues.keyNavigationWraps
                    caption: qsTr("Navigation Wraps")
                    toolTip: qsTr("This property holds whether the grid wraps key navigation.")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 1000;
                }
                //                Qt namespace enums not supported by the rewriter
                //                QWidget {
                //                    layout: HorizontalLayout {
                //                        Label {
                //                            text: qsTr("Layout Direction")
                //                        }

                //                        ComboBox {
                //                            baseStateFlag: isBaseState
                //                            items : { ["LeftToRight", "TopToBottom"] }
                //                            currentText: backendValues.layoutDirection.value;
                //                            onItemsChanged: {
                //                                currentText =  backendValues.layoutDirection.value;
                //                            }
                //                            backendValue: backendValues.layoutDirection
                //                        }
                //                    }
                //                } //QWidget

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Orientation")
                            toolTip: qsTr("This property holds the orientation of the list.")
                        }

                        ComboBox {
                            baseStateFlag: isBaseState
                            items : { ["Horizontal", "Vertical"] }
                            currentText: backendValues.snapMode.value;
                            onItemsChanged: {
                                currentText =  backendValues.snapMode.value;
                            }
                            backendValue: backendValues.snapMode
                        }
                    }
                } //QWidget
                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Snap Mode")
                            toolTip: qsTr("This property determines how the view scrolling will settle following a drag or flick.")
                        }

                        ComboBox {
                            baseStateFlag: isBaseState
                            items : { ["NoSnap", "SnapToRow", "SnapOneRow"] }
                            currentText: backendValues.snapMode.value;
                            onItemsChanged: {
                                currentText =  backendValues.snapMode.value;
                            }
                            backendValue: backendValues.snapMode
                        }
                    }
                } //QWidget
                IntEditor {
                    backendValue: backendValues.spacing
                    caption: qsTr("Spacing")
                    toolTip: qsTr("This property holds the spacing between items.")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 1000;
                }
            }
        }
        GroupBox {
            finished: finishedNotify;
            caption: qsTr("List View Highlight")
            layout: VerticalLayout {
                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Range")
                            toolTip: qsTr("Highlight Range")
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
                    toolTip: qsTr("This property holds the move animation duration of the highlight delegate.")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 1000;
                }
                IntEditor {
                    backendValue: backendValues.highlightMoveSpeed
                    caption: qsTr("Move Speed")
                    toolTip: qsTr("This property holds the move animation speed of the highlight delegate.")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 1000;
                }
                IntEditor {
                    backendValue: backendValues.highlightResizeDuration
                    caption: qsTr("Resize Duration")
                    toolTip: qsTr("This property holds the resize animation duration of the highlight delegate.")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 1000;
                }
                IntEditor {
                    backendValue: backendValues.highlightResizeSpeed
                    caption: qsTr("Resize Speed")
                    toolTip: qsTr("This property holds the resize animation speed of the highlight delegate.")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 1000;
                }
                IntEditor {
                    backendValue: backendValues.preferredHighlightBegin
                    caption: qsTr("Preferred Begin")
                    toolTip: qsTr("Preferred Highlight Begin - must be smaller than Preferred Highlight End")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 1000;
                }
                IntEditor {
                    backendValue: backendValues.preferredHighlightEnd
                    caption: qsTr("Preferred End")
                    toolTip: qsTr("Preferred Highlight End - must be larger than Preferred Highlight End")
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
                            toolTip: qsTr("This property sets whether the highlight is managed by the view.")
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
