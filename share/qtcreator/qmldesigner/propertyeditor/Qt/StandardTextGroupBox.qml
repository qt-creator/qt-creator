import Qt 4.7
import Bauhaus 1.0

GroupBox {
    id: standardTextGroupBox

    caption: qsTr("Text")

    property bool showIsWrapping: false
    property bool showVerticalAlignment: false

    layout: VerticalLayout {
        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: qsTr("Text")
                }
                LineEdit {
                    backendValue: backendValues.text
                    baseStateFlag: isBaseState;
                }
            }
        }
        QWidget {
            visible: showIsWrapping
            layout: HorizontalLayout {
                Label {
                    text: qsTr("Wrap Mode")
                }
                ComboBox {
                    baseStateFlag: isBaseState
                    minimumHeight: 22;
                    items : { ["NoWrap", "WordWrap", "WrapAnywhere", "WrapAtWordBoundaryOrAnywhere"] }
                    currentText: backendValues.wrapMode.value;
                    onItemsChanged: {
                        currentText =  backendValues.wrapMode.value;
                    }
                    backendValue: backendValues.wrapMode
                }
            }
        }
        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: qsTr("Alignment")
                }
                AlignmentHorizontalButtons {}
            }
        }
        QWidget {
            visible: showVerticalAlignment
            layout: HorizontalLayout {

                Label {
                    text: qsTr("")
                }
                AlignmentVerticalButtons { }
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
    }
}
