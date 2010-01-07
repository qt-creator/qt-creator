import Qt 4.6
import Bauhaus 1.0

GroupBox {
    maximumHeight: 200;

    finished: finishedNotify;
    caption: "WebView";
    id: webViewSpecifics;

    layout: QVBoxLayout {

        topMargin: 18;
        bottomMargin: 2;
        leftMargin: 8;
        rightMargin: 8;
        QWidget {
            layout: QHBoxLayout {
                leftMargin: 0;
                rightMargin: 0;
                QLabel {
                    minimumHeight: 20;
                    text: "Url:"
                    font.bold: true;
                    }
                    QLineEdit {
                        text: backendValues.url.value;
                        onEditingFinished: {
                            backendValues.url.value = text;
                        }
                    }
                }
            }



            IntEditor {
                id: preferredWidth;
                backendValue: backendValues.preferredWidth;
                caption: "Prefered Width:  "
                baseStateFlag: isBaseState;
                step: 1;
                minimumValue: 0;
                maximumValue: 2000;
            }

			 IntEditor {
                id: webPageWidth;
                backendValue: backendValues.preferredHeight;
                caption: "Web Page Height:"
                baseStateFlag: isBaseState;
                step: 1;
                minimumValue: 0;
                maximumValue: 2000;
            }

			QWidget {
                            layout: QHBoxLayout {
                                topMargin: 10;
                                bottomMargin: 0;
                                leftMargin: 0;
                                rightMargin: 0;
								spacing: 20;

								QLabel {
									minimumHeight: 20;
									text: "ZommFactor:"
									font.bold: true;
								}
                                DoubleSpinBox {
                                    id: ZoomSpinBox;
                                    objectName: "ZommSpinBox";
                                    backendValue: backendValues.zoomFactor;
                                    minimumWidth: 60;
                                    minimum: 0.01
                                    maximum: 10
                                    singleStep: 0.1
                                    baseStateFlag: isBaseState;
                                    onBackendValueChanged: {
                                       ZoomSlider.value = backendValue.value * 10;
                                    }
                                }
                                QSlider {
                                    id: ZoomSlider;
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
