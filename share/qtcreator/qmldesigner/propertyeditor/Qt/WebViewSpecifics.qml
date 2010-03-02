import Qt 4.6
import Bauhaus 1.0

QWidget {
    layout: QVBoxLayout {
        topMargin: 0
        bottomMargin: 0
        leftMargin: 0
        rightMargin: 0
        spacing: 0
        GroupBox {


            GroupBox {
                maximumHeight: 200;

                finished: finishedNotify;
                caption: "WebView";
                id: webViewSpecifics;

                layout: VerticalLayout {
                    QWidget {
                        layout: HorizontalLayout {
                            leftMargin: 0;
                            rightMargin: 0;
                            Label {
                                text: "Url"
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
                        caption: "Prefered Width"
                        baseStateFlag: isBaseState;
                        step: 1;
                        minimumValue: 0;
                        maximumValue: 2000;
                    }

                    IntEditor {
                        id: webPageWidth;
                        backendValue: backendValues.preferredHeight;
                        caption: "Page Height"
                        baseStateFlag: isBaseState;
                        step: 1;
                        minimumValue: 0;
                        maximumValue: 2000;
                    }

                    QWidget {
                        layout: HorizontalLayout {

                            Label {
                                minimumHeight: 20;
                                text: "Zoom Factor"
                            }
                            DoubleSpinBox {
                                id: zoomSpinBox;
                                minimumWidth: 60;
                                text: ""
                                backendValue: backendValues.zoomFactor;
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
                                    backendValues.zoomFactor.value = value / 10;
                                }
                            }
                        }
                    }
                }
            }

        }
    }
}
