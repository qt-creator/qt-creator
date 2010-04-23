import Qt 4.7
import Bauhaus 1.0

GroupBox {
    id: rectangleColorGroupBox

    caption: qsTr("Colors")


    property bool selectionFlag: selectionChanged

    property bool isSetup;

    property bool hasBorder


    onSelectionFlagChanged: {
        isSetup = true;
        gradientLine.active = false;
        colorGroupBox.setSolidButtonChecked = true;
        if (backendValues.gradient.isInModel) {
            colorGroupBox.setGradientButtonChecked = true;
            gradientLine.active = true;
            gradientLine.setupGradient();
        }
        if (colorGroupBox.alpha == 0) {
            colorGroupBox.setNoneButtonChecked = true;
            //borderColorBox.collapseBox = true
        }

        if (backendValues.border_color.isInModel || backendValues.border_width.isInModel) {
            borderColorBox.setSolidButtonChecked = true;
            hasBorder = true;
        } else {
            borderColorBox.setNoneButtonChecked = true;
            hasBorder = false
            //borderColorBox.collapseBox = true
        }

        isSetup = false
    }

    layout: VerticalLayout {

        QWidget {
            visible: colorGroupBox.gradientButtonChecked
            layout: HorizontalLayout {
                spacing: 2
                Label {
                    text: qsTr("Stops")
                    toolTip: qsTr("Gradient Stops")
                }

                GradientLine {
                    id: gradientLine;
                    activeColor: colorGroupBox.color
                    itemNode: anchorBackend.itemNode
                    active: colorGroupBox.gradientButtonChecked
                }
            }
        }

        ColorGroupBox {
            id: colorGroupBox
            caption: qsTr("Rectangle")
            finished: finishedNotify
            backendColor: backendValues.color
            //gradientEditing: true;
            gradientColor: gradientLine.activeColor;
            showButtons: true;
            showGradientButton: true;

            onGradientButtonCheckedChanged: {
                if (isSetup)
                    return;
                if (gradientButtonChecked) {
                    gradientLine.active = true
                    gradientLine.setupGradient();
                    backendValues.color.resetValue();
                } else {
                    gradientLine.active = false
                    if (backendValues.gradient.isInModel)
                        gradientLine.deleteGradient();

                }
            }

            onNoneButtonCheckedChanged: {
                if (isSetup)
                    return;
                if (noneButtonChecked) {
                    backendValues.color.value = "transparent";
                    collapseBox = false
                } else {
                    alpha = 255;
                }
            }

            onSolidButtonCheckedChanged: {
                if (isSetup)
                    return;
                if (solidButtonChecked) {
                    backendValues.color.resetValue();
                }
            }
        }

        ColorGroupBox {
            id: borderColorBox
            caption: qsTr("Border")
            finished: finishedNotify
            showButtons: true;

            backendColor: backendValues.border_color

            property variant backendColorValue: backendValues.border_color.vlaue
            enabled: isBaseState || hasBorder

            onBackendColorValueChanged: {
                if (backendValues.border_color.isInModel)
                    borderColorBox.setNoneButtonChecked = true;
            }

            onNoneButtonCheckedChanged: {
                if (isSetup)
                    return;
                if (noneButtonChecked) {
                    transaction.start();
                    backendValues.border_color.resetValue();
                    backendValues.border_width.resetValue();
                    transaction.end();
                    hasBorder = false
                    collapseBox = false
                } else {
                    transaction.start();
                    backendValues.border_color.value = "blue"
                    backendValues.border_color.value = "#000000";
                    transaction.end();
                    hasBorder = true
                }
            }

            onSolidButtonCheckedChanged: {
                if (isSetup)
                    return;
                if (solidButtonChecked) {
                    transaction.start();
                    backendValues.border_color.value = "blue"
                    backendValues.border_color.value = "#000000";
                    transaction.end();
                    hasBorder = true
                }
            }
        }

    }

}
