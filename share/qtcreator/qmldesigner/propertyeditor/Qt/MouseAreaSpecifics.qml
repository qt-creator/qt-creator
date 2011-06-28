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
            caption: qsTr("MouseArea")

            layout: VerticalLayout {

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Enabled")
                            toolTip: qsTr("This property holds whether the item accepts mouse events.")
                        }

                        CheckBox {
                            text: ""
                            toolTip: qsTr("This property holds whether the item accepts mouse events.")
                            backendValue: backendValues.enabled
                            baseStateFlag: isBaseState
                            checkable: true
                        }
                    }
                } //QWidget
                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Hover Enabled")
                            toolTip: qsTr("This property holds whether hover events are handled.")
                        }

                        CheckBox {
                            text: ""
                            toolTip: qsTr("This property holds whether hover events are handled.")
                            backendValue: backendValues.hoverEnabled
                            baseStateFlag: isBaseState
                            checkable: true
                        }
                    }
                } //QWidget
            }
        }
    }
}

