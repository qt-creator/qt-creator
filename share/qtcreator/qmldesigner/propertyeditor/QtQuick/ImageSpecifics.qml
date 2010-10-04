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
            maximumHeight: 240;

            finished: finishedNotify;
            caption: qsTr("Image")

            layout: VerticalLayout {

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Source")
                        }

                        FileWidget {
                            enabled: isBaseState || backendValues.id.value != "";
                            fileName: backendValues.source.value;
                            onFileNameChanged: {
                                backendValues.source.value = fileName;
                            }
                            itemNode: anchorBackend.itemNode
                            filter: "*.png *.gif *.jpg"
                            showComboBox: true
                        }
                    }
                }

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Fill Mode")
                        }

                        ComboBox {
                            baseStateFlag: isBaseState
                            items : { ["Stretch", "PreserveAspectFit", "PreserveAspectCrop", "Tile", "TileVertically", "TileHorizontally"] }
                            currentText: backendValues.fillMode.value;
                            onItemsChanged: {
                                currentText =  backendValues.fillMode.value;
                            }
                            backendValue: backendValues.fillMode
                        }
                    }
                }

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Aliasing")
                        }

                        CheckBox {
                            text: qsTr("Smooth")
                            backendValue: backendValues.smooth
                            baseStateFlag: isBaseState;
                            checkable: true;
                        }
                    }
                }

                QWidget {  // 1
                    layout: HorizontalLayout {

                        Label {
                            text: qsTr("Source Size")
                        }

                        DoubleSpinBox {
                            text: "W"
                            alignRight: false
                            spacing: 4
                            singleStep: 1;
                            enabled: anchorBackend.hasParent;
                            backendValue: backendValues.sourceSize_width
                            minimum: -2000;
                            maximum: 2000;
                            baseStateFlag: isBaseState;
                        }

                        DoubleSpinBox {
                            singleStep: 1;
                            text: "H"
                            alignRight: false
                            spacing: 4
                            backendValue: backendValues.sourceSize_height
                            enabled: anchorBackend.hasParent;
                            minimum: -2000;
                            maximum: 2000;
                            baseStateFlag: isBaseState;
                        }


                    }
                } //QWidget  //1

                QWidget {  // 1
                    layout: HorizontalLayout {

                        Label {
                            text: qsTr("Painted Size")
                        }

                        DoubleSpinBox {
                            text: "W"
                            alignRight: false
                            spacing: 4
                            singleStep: 1;
                            enabled: anchorBackend.hasParent;
                            backendValue: backendValues.paintedWidth
                            minimum: -2000;
                            maximum: 2000;
                            baseStateFlag: isBaseState;
                        }

                        DoubleSpinBox {
                            singleStep: 1;
                            text: "H"
                            alignRight: false
                            spacing: 4
                            backendValue: backendValues.paintedHeight
                            enabled: anchorBackend.hasParent;
                            minimum: -2000;
                            maximum: 2000;
                            baseStateFlag: isBaseState;
                        }


                    }
                } //QWidget  //1

            }
        }

        QScrollArea {}
    }
}
