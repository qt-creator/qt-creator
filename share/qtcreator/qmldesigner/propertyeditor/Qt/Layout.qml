import Qt 4.6
import Bauhaus 1.0

GroupBox {
    finished: finishedNotify;

    caption: "Layout";

    id: layout;
    enabled: anchorBackend.hasParent;

    property var targetLabelWidth: 90 - 20 - 26

    layout: VerticalLayout {
        QLabel {
            text: "Anchors"
        }
        QWidget {
            layout: HorizontalLayout {
                leftMargin: 10


                AnchorButtons {}
            }
        }

        QWidget {
            visible: anchorBackend.topAnchored;
            layout : VerticalLayout {
                topMargin: 8;
                bottomMargin: 4;
                rightMargin: 20;
                QWidget {
                    layout: HorizontalLayout {
                        leftMargin: 10
                        QLabel {
                            //iconFromFile: "qrc:qmldesigner/images/icon-top.png"
                            fixedWidth: 32
                            fixedHeight: 32
                            styleSheet: "border-image: url(:/qmldesigner/images/icon-top.png)";
                        }

                        Label {
                            text: "Target"
                            fixedWidth: targetLabelWidth
                        }
                        SiblingComboBox {
                            itemNode: anchorBackend.itemNode
                            selectedItemNode: anchorBackend.topTarget
                            onSelectedItemNodeChanged: { anchorBackend.topTarget = selectedItemNode; }
                        }

                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        IntEditor {
                            id:topbox
                            slider: false
                            caption: "Margin"
                            backendValue: backendValues.anchors_topMargin
                            baseStateFlag: isBaseState;
                            maximumValue: 1000
                            minimumValue: -1000
                        }

                        PlaceHolder {
                            fixedWidth: 140
                        }
                    }
                }
            }
        }
        QWidget {
            visible: anchorBackend.bottomAnchored;
            layout : VerticalLayout {
                topMargin: 8;
                bottomMargin: 4;
                rightMargin: 20;
                QWidget {
                    layout: HorizontalLayout {
                        leftMargin: 10
                        QLabel {
                            //iconFromFile: "qrc:qmldesigner/images/icon-top.png"
                            fixedWidth: 32
                            fixedHeight: 32
                            styleSheet: "border-image: url(:/qmldesigner/images/icon-bottom.png)";
                        }

                        Label {
                            text: "Target"
                            fixedWidth: targetLabelWidth
                        }
                        SiblingComboBox {
                            itemNode: anchorBackend.itemNode
                            selectedItemNode: anchorBackend.bottomTarget
                            onSelectedItemNodeChanged: { anchorBackend.bottomTarget = selectedItemNode; }
                        }

                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        IntEditor {
                            slider: false
                            caption: "Margin"
                            backendValue: backendValues.anchors_bottomMargin
                            baseStateFlag: isBaseState;
                            maximumValue: 1000
                            minimumValue: -1000
                        }

                        PlaceHolder {
                            fixedWidth: 140
                        }
                    }
                }
            }
        }
        QWidget {
            visible: anchorBackend.leftAnchored;
            layout : VerticalLayout {
                topMargin: 8;
                bottomMargin: 4;
                rightMargin: 20;
                QWidget {
                    layout: HorizontalLayout {
                        leftMargin: 10
                        QLabel {
                            //iconFromFile: "qrc:qmldesigner/images/icon-top.png"
                            fixedWidth: 32
                            fixedHeight: 32
                            styleSheet: "border-image: url(:/qmldesigner/images/icon-left.png)";
                        }

                        Label {
                            text: "Target"
                            fixedWidth: targetLabelWidth
                        }
                        SiblingComboBox {
                            itemNode: anchorBackend.itemNode
                            selectedItemNode: anchorBackend.leftTarget
                            onSelectedItemNodeChanged: { anchorBackend.leftTarget = selectedItemNode; }
                        }
                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        IntEditor {
                            slider: false
                            caption: "Margin"
                            backendValue: backendValues.anchors_leftMargin
                            baseStateFlag: isBaseState;
                            maximumValue: 1000
                            minimumValue: -1000
                        }

                        PlaceHolder {
                            fixedWidth: 140
                        }
                    }
                }
            }
        }
        QWidget {
            visible: anchorBackend.rightAnchored;
            layout : VerticalLayout {
                topMargin: 8;
                bottomMargin: 4;
                rightMargin: 20;
                QWidget {
                    layout: HorizontalLayout {
                        leftMargin: 10
                        QLabel {
                            //iconFromFile: "qrc:qmldesigner/images/icon-top.png"
                            fixedWidth: 32
                            fixedHeight: 32
                            styleSheet: "border-image: url(:/qmldesigner/images/icon-right.png)";
                        }

                        Label {
                            text: "Target"
                            fixedWidth: targetLabelWidth
                        }
                        SiblingComboBox {
                            itemNode: anchorBackend.itemNode
                            selectedItemNode: anchorBackend.rightTarget
                            onSelectedItemNodeChanged: { anchorBackend.rightTarget = selectedItemNode; }
                        }
                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        IntEditor {
                            slider: false
                            caption: "Margin"
                            backendValue: backendValues.anchors_rightMargin
                            baseStateFlag: isBaseState;
                            maximumValue: 1000
                            minimumValue: -1000
                        }

                        PlaceHolder {
                            fixedWidth: 140
                        }
                    }
                }
            }
        }
        QWidget {
            visible: anchorBackend.horizontalCentered
            layout : VerticalLayout {
                topMargin: 8;
                bottomMargin: 4;
                rightMargin: 20;
                QWidget {
                    layout: HorizontalLayout {
                        leftMargin: 10
                        QLabel {
                            //iconFromFile: "qrc:qmldesigner/images/icon-top.png"
                            fixedWidth: 32
                            fixedHeight: 32
                            styleSheet: "border-image: url(:/qmldesigner/images/icon-horizontal.png)";
                        }

                        Label {
                            text: "Target"
                            fixedWidth: targetLabelWidth
                        }
                        SiblingComboBox {
                            itemNode: anchorBackend.itemNode
                            selectedItemNode: anchorBackend.horizontalTarget
                            onSelectedItemNodeChanged: { anchorBackend.horizontalTarget = selectedItemNode; }
                        }
                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        IntEditor {
                            slider: false
                            caption: "Margin"
                            baseStateFlag: isBaseState;
                            backendValue: backendValues.anchors_horizontalCenterOffset
                            maximumValue: 1000
                            minimumValue: -1000
                        }

                        PlaceHolder {
                            fixedWidth: 140
                        }
                    }
                }
            }
        }
        QWidget {
            visible: anchorBackend.verticalCentered
            layout : VerticalLayout {
                topMargin: 8;
                bottomMargin: 4;
                rightMargin: 20;
                QWidget {
                    layout: HorizontalLayout {
                        leftMargin: 10
                        QLabel {
                            //iconFromFile: "qrc:qmldesigner/images/icon-top.png"
                            fixedWidth: 32
                            fixedHeight: 32
                            styleSheet: "border-image: url(:/qmldesigner/images/icon-vertical.png)";
                        }

                        Label {
                            text: "Target"
                            fixedWidth: targetLabelWidth
                        }
                        SiblingComboBox {
                            itemNode: anchorBackend.itemNode
                            selectedItemNode: anchorBackend.verticalTarget
                            onSelectedItemNodeChanged: { anchorBackend.verticalTarget = selectedItemNode; }
                        }
                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        IntEditor {
                            slider: false
                            caption: "Margin"
                            backendValue: backendValues.anchors_verticalCenterOffset
                            baseStateFlag: isBaseState;
                            maximumValue: 1000
                            minimumValue: -1000
                        }

                        PlaceHolder {
                            fixedWidth: 140
                        }
                    }
                }
            }
        }
    }
}
