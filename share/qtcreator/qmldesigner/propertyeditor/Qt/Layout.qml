import Qt 4.7
import Bauhaus 1.0

GroupBox {
    finished: finishedNotify;

    caption: qsTr("Layout")

    id: layout;
    enabled: anchorBackend.hasParent;

    property bool isInBaseState: isBaseState

    property variant targetLabelWidth: 90 - 20 - 26
    property int leftMarginMargin: 16

    layout: VerticalLayout {
        Label {
            text: qsTr("Anchors")
        }
        QWidget {
            layout: HorizontalLayout {
                leftMargin: 10
                topMargin: 8


                AnchorButtons {
                    //opacity: enabled?1.0:0.5;
                    enabled: isInBaseState
                    fixedWidth:266
                }
            }
        }

        QWidget {
            visible: anchorBackend.topAnchored;
            layout : VerticalLayout {
                topMargin: 8;
                bottomMargin: 4;
                rightMargin: 10;
                QWidget {
                    layout: HorizontalLayout {
                        leftMargin: 10
                        QLabel {
                            //iconFromFile: "qrc:qmldesigner/images/icon-top.png"
                            fixedWidth: 16
                            fixedHeight: 16
                            styleSheet: "border-image: url(:/qmldesigner/images/icon-top.png)";
                        }

                        Label {
                            text: qsTr("Target")
                            fixedWidth: targetLabelWidth
                        }
                        SiblingComboBox {
                            enabled: isInBaseState
                            itemNode: anchorBackend.itemNode
                            selectedItemNode: anchorBackend.topTarget
                            onSelectedItemNodeChanged: { anchorBackend.topTarget = selectedItemNode; }
                        }

                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        leftMargin: leftMarginMargin
                        IntEditor {
                            id:topbox
                            slider: false
                            caption: qsTr("Margin")
                            backendValue: backendValues.anchors_topMargin
                            baseStateFlag: isInBaseState;
                            maximumValue: 1000
                            minimumValue: -1000
                        }

                        PlaceHolder {
                            fixedWidth: 80
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
                rightMargin: 10;
                QWidget {
                    layout: HorizontalLayout {
                        leftMargin: 10
                        QLabel {
                            //iconFromFile: "qrc:qmldesigner/images/icon-top.png"
                            fixedWidth: 16
                            fixedHeight: 16
                            styleSheet: "border-image: url(:/qmldesigner/images/icon-bottom.png)";
                        }

                        Label {
                            text: qsTr("Target")
                            fixedWidth: targetLabelWidth
                        }
                        SiblingComboBox {
                            enabled: isInBaseState
                            itemNode: anchorBackend.itemNode
                            selectedItemNode: anchorBackend.bottomTarget
                            onSelectedItemNodeChanged: { anchorBackend.bottomTarget = selectedItemNode; }
                        }

                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        leftMargin: leftMarginMargin
                        IntEditor {
                            slider: false
                            caption: qsTr("Margin")
                            backendValue: backendValues.anchors_bottomMargin
                            baseStateFlag: isInBaseState;
                            maximumValue: 1000
                            minimumValue: -1000
                        }

                        PlaceHolder {
                            fixedWidth: 80
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
                rightMargin: 10;
                QWidget {
                    layout: HorizontalLayout {
                        leftMargin: 10
                        QLabel {
                            //iconFromFile: "qrc:qmldesigner/images/icon-top.png"
                            fixedWidth: 16
                            fixedHeight: 16
                            styleSheet: "border-image: url(:/qmldesigner/images/icon-left.png)";
                        }

                        Label {
                            text: qsTr("Target")
                            fixedWidth: targetLabelWidth
                        }
                        SiblingComboBox {
                            enabled: isInBaseState
                            itemNode: anchorBackend.itemNode
                            selectedItemNode: anchorBackend.leftTarget
                            onSelectedItemNodeChanged: { anchorBackend.leftTarget = selectedItemNode; }
                        }
                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        leftMargin: leftMarginMargin
                        IntEditor {
                            slider: false
                            caption: qsTr("Margin")
                            backendValue: backendValues.anchors_leftMargin
                            baseStateFlag: isInBaseState;
                            maximumValue: 1000
                            minimumValue: -1000
                        }

                        PlaceHolder {
                            fixedWidth: 80
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
                rightMargin: 10;
                QWidget {
                    layout: HorizontalLayout {
                        leftMargin: 10
                        QLabel {
                            //iconFromFile: "qrc:qmldesigner/images/icon-top.png"
                            fixedWidth: 16
                            fixedHeight: 16
                            styleSheet: "border-image: url(:/qmldesigner/images/icon-right.png)";
                        }

                        Label {
                            text: qsTr("Target")
                            fixedWidth: targetLabelWidth
                        }
                        SiblingComboBox {
                            enabled: isInBaseState
                            itemNode: anchorBackend.itemNode
                            selectedItemNode: anchorBackend.rightTarget
                            onSelectedItemNodeChanged: { anchorBackend.rightTarget = selectedItemNode; }
                        }
                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        leftMargin: leftMarginMargin
                        IntEditor {
                            slider: false
                            caption: qsTr("Margin")
                            backendValue: backendValues.anchors_rightMargin
                            baseStateFlag: isInBaseState;
                            maximumValue: 1000
                            minimumValue: -1000
                        }

                        PlaceHolder {
                            fixedWidth: 80
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
                rightMargin: 10;
                QWidget {
                    layout: HorizontalLayout {
                        leftMargin: 10
                        QLabel {
                            //iconFromFile: "qrc:qmldesigner/images/icon-top.png"
                            fixedWidth: 16
                            fixedHeight: 16
                            styleSheet: "border-image: url(:/qmldesigner/images/icon-horizontal.png)";
                        }

                        Label {
                            text: qsTr("Target")
                            fixedWidth: targetLabelWidth
                        }
                        SiblingComboBox {
                            enabled: isInBaseState
                            itemNode: anchorBackend.itemNode
                            selectedItemNode: anchorBackend.horizontalTarget
                            onSelectedItemNodeChanged: { anchorBackend.horizontalTarget = selectedItemNode; }
                        }
                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        leftMargin: leftMarginMargin
                        IntEditor {
                            slider: false
                            caption: qsTr("Margin")
                            baseStateFlag: isInBaseState;
                            backendValue: backendValues.anchors_horizontalCenterOffset
                            maximumValue: 1000
                            minimumValue: -1000
                        }

                        PlaceHolder {
                            fixedWidth: 80
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
                rightMargin: 10;
                QWidget {
                    layout: HorizontalLayout {
                        leftMargin: 10
                        QLabel {
                            //iconFromFile: "qrc:qmldesigner/images/icon-top.png"
                            fixedWidth: 16
                            fixedHeight: 16
                            styleSheet: "border-image: url(:/qmldesigner/images/icon-vertical.png)";
                        }

                        Label {
                            text: qsTr("Target")
                            fixedWidth: targetLabelWidth
                        }
                        SiblingComboBox {
                            enabled: isInBaseState
                            itemNode: anchorBackend.itemNode
                            selectedItemNode: anchorBackend.verticalTarget
                            onSelectedItemNodeChanged: { anchorBackend.verticalTarget = selectedItemNode; }
                        }
                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        leftMargin: leftMarginMargin
                        IntEditor {
                            slider: false
                            caption: qsTr("Margin")
                            backendValue: backendValues.anchors_verticalCenterOffset
                            baseStateFlag: isInBaseState;
                            maximumValue: 1000
                            minimumValue: -1000
                        }

                        PlaceHolder {
                            fixedWidth: 80
                        }
                    }
                }
            }
        }
    }
}
