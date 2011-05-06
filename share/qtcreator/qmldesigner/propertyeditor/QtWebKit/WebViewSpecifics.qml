import Qt 4.7
import Bauhaus 1.0
import "../Qt/"

QWidget {
    layout: QVBoxLayout {
        topMargin: 0
        bottomMargin: 0
        leftMargin: 0
        rightMargin: 0
        spacing: 0

        GroupBox {
            maximumHeight: 200;

            finished: finishedNotify;
            caption: qsTr("WebView")
            id: webViewSpecifics;

            layout: VerticalLayout {
                QWidget {
                    layout: HorizontalLayout {
                        leftMargin: 0;
                        rightMargin: 0;
                        Label {
                            text: qsTr("Url")
                        }
                        LineEdit {
                            backendValue: backendValues.url
                            baseStateFlag: isBaseState;
                        }
                    }
                }                       

                IntEditor {
                    id: preferredWidth;
                    backendValue: backendValues.preferredWidth;
                    caption: qsTr("Pref Width")
                    toolTip: qsTr("Preferred Width")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 2000;
                }

                IntEditor {
                    id: webPageWidth;
                    backendValue: backendValues.preferredHeight;
                    caption: qsTr("Pref Height")
                    toolTip: qsTr("Preferred Height")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 2000;
                }

                QWidget {
                    layout: HorizontalLayout {

                        Label {
                            text: qsTr("Scale")
                            toolTip: qsTr("Contents Scale")
                        }
                        DoubleSpinBox {
                            id: zoomSpinBox;
                            minimumWidth: 60;
                            text: ""
                            backendValue: backendValues.contentsScale;
                            minimum: 0.01
                            maximum: 10
                            singleStep: 0.1
                            baseStateFlag: isBaseState;
                            onBackendValueChanged: {
                                zoomSlider.value = backendValue.value * 10;
                            }
                        }
                        QSlider {
                            id: zoomSlider;
                            orientation: "Qt::Horizontal";
                            minimum: 1;
                            maximum: 100;
                            singleStep: 1;
                            onValueChanged: {
                                backendValues.contentsScale.value = value / 10;
                            }
                        }
                    }
                }
            }
        }
    }
}
