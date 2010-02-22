import Qt 4.6
import Bauhaus 1.0

GroupBox {
    finished: finishedNotify;

    caption: "layout";

    id: layout;
    enabled: anchorBackend.hasParent;

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
                            fixedWidth: 90 - 20 - 32
                        }
                        QComboBox {
                        }

                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        IntEditor {
                            slider: false
                            caption: "Margin"
							backendValue: backendValues.anchors_topMargin
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
                            fixedWidth: 90 - 20 - 32
                        }
                        QComboBox {
                        }

                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        IntEditor {
                            slider: false
                            caption: "Margin"
							backendValue: backendValues.anchors_bottomMargin
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
                            fixedWidth: 90 - 20 - 32
                        }
                        QComboBox {
                        }
                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        IntEditor {
                            slider: false
                            caption: "Margin"
							backendValue: backendValues.anchors_leftMargin
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
                            fixedWidth: 90 - 20 - 32
                        }
                        QComboBox {
                        }
                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        IntEditor {
                            slider: false
                            caption: "Margin"
							backendValue: backendValues.anchors_rightMargin
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
                            fixedWidth: 90 - 20 - 32
                        }
                        QComboBox {
                        }
                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        IntEditor {
                            slider: false
                            caption: "Margin"
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
                            fixedWidth: 90 - 20 - 32
                        }
                        QComboBox {
                        }
                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        IntEditor {
                            slider: false
                            caption: "Margin"
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
