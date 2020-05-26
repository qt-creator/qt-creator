/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.1
import QtQuick.Layouts 1.0
import QtQuickDesignerTheme 1.0
import QtQuick.Dialogs 1.3
import StudioControls 1.0 as StudioControls

Column {
    id: colorEditor

    width: parent.width - 8

    property color color

    property bool supportGradient: false

    property string caption: "Color"

    property variant backendValue

    property variant value: {
        if (isVector3D)
            return Qt.rgba(backendValue.value.x, backendValue.value.y, backendValue.value.z, 1);
        else
            return backendValue.value;

    }

    property alias gradientPropertyName: gradientLine.gradientPropertyName

    property bool shapeGradients: false

    property alias transparent: transparentButton.checked

    property color originalColor

    property bool isVector3D: false

    function isNotInGradientMode() {
        return (buttonRow.checkedIndex === 0 || transparent)
    }

    function resetShapeColor() {
        colorEditor.backendValue.resetValue()
    }

    onValueChanged: colorEditor.color = colorEditor.value

    onBackendValueChanged: colorEditor.color = colorEditor.value

    Timer {
        id: colorEditorTimer
        repeat: false
        interval: 100
        running: false
        onTriggered: {
            if (colorEditor.backendValue !== undefined) {
                if (isVector3D) {
                    colorEditor.backendValue.value = Qt.vector3d(colorEditor.color.r,
                                                                 colorEditor.color.g,
                                                                 colorEditor.color.b);
                } else {
                    colorEditor.backendValue.value = colorEditor.color;
                }
            }
        }
    }

    onColorChanged: {
        if (!gradientLine.isInValidState)
            return;

        if (colorEditor.supportGradient && gradientLine.hasGradient) {
            textField.text = convertColorToString(color)
            gradientLine.currentColor = color
        }

        if (isNotInGradientMode()) {
            //Delay setting the color to keep ui responsive
            colorEditorTimer.restart()
        }

        colorPalette.selectedColor = color
    }

    ColorLine {
        visible: {
            return (colorEditor.supportGradient && isNotInGradientMode())
        }
        currentColor: colorEditor.color
        width: parent.width
    }

    GradientLine {
        property bool isInValidState: false
        visible: {
            return !(isNotInGradientMode())
        }
        id: gradientLine

        width: parent.width

        onCurrentColorChanged: {
            if (colorEditor.supportGradient && gradientLine.hasGradient) {
                colorEditor.color = gradientLine.currentColor
            }
        }

        onHasGradientChanged: {
            if (!colorEditor.supportGradient)
                return

            if (gradientLine.hasGradient) {
                if (colorEditor.shapeGradients) {
                    switch (gradientLine.gradientTypeName) {
                    case "LinearGradient":
                        buttonRow.initalChecked = 1
                        break;
                    case "RadialGradient":
                        buttonRow.initalChecked = 2
                        break;
                    case "ConicalGradient":
                        buttonRow.initalChecked = 3
                        break;
                    default:
                        buttonRow.initalChecked = 1
                    }
                } else {
                    buttonRow.initalChecked = 1
                }
                colorEditor.color = gradientLine.currentColor
            } else if (colorEditor.transparent) {
                buttonRow.initalChecked = 4
            } else {
                buttonRow.initalChecked = 0
                colorEditor.color = colorEditor.value
            }

            buttonRow.checkedIndex = buttonRow.initalChecked
            colorEditor.originalColor = colorEditor.color
        }

        onSelectedNodeChanged: {
            if (colorEditor.supportGradient && gradientLine.hasGradient) {
                colorEditor.originalColor = gradientLine.currentColor
            }
        }

        Connections {
            target: modelNodeBackend
            onSelectionToBeChanged: {
                colorEditorTimer.stop()
                gradientLine.isInValidState = false
                if (colorEditor.originalColor !== colorEditor.color) {
                    if (colorEditor.color != "#ffffff"
                            && colorEditor.color != "#000000"
                            && colorEditor.color != "#00000000") {
                        colorPalette.addColorToPalette(colorEditor.color)
                    }
                }
            }
        }

        Connections {
            target: modelNodeBackend
            onSelectionChanged: {
                if (colorEditor.supportGradient && gradientLine.hasGradient) {
                    colorEditor.color = gradientLine.currentColor
                    gradientLine.currentColor = color
                    textField.text = colorEditor.color
                }
                gradientLine.isInValidState = true
                colorEditor.originalColor = colorEditor.color
                colorPalette.selectedColor = colorEditor.color
            }
        }

    }

    SectionLayout {
        width: parent.width
        columnSpacing: 0
        rowSpacing: checkButton.checked ? 8 : 2

        rows: 5

        //spacer 1
        Item {
            height: 6
        }

        SecondColumnLayout {

            ColorCheckButton {
                id: checkButton
                buttonColor: colorEditor.color

                onCheckedChanged: {
                    if (contextMenu.opened)
                        contextMenu.close()
                }
                onRightMouseButtonClicked: contextMenu.popup(checkButton)
            }

            LineEdit {
                enabled: !colorEditor.transparent
                id: textField

                writeValueManually: true

                validator: RegExpValidator {
                    regExp: /#[0-9A-Fa-f]{6}([0-9A-Fa-f]{2})?/g
                }

                showTranslateCheckBox: false

                backendValue: colorEditor.backendValue

                onAccepted: {
                    colorEditor.color = colorFromString(textField.text)
                }

                onCommitData: {
                    colorEditor.color = colorFromString(textField.text)
                    if (isNotInGradientMode()) {
                        if (colorEditor.isVector3D) {
                            backendValue.value = Qt.vector3d(colorEditor.color.r,
                                                             colorEditor.color.g,
                                                             colorEditor.color.b);
                        } else {
                            backendValue.value = colorEditor.color;
                        }
                    }
                }

                Layout.fillWidth: true
            }

            ButtonRow {

                id: buttonRow
                exclusive: true

                ButtonRowButton {
                    iconSource: "images/icon_color_solid.png"

                    onClicked: {
                        gradientLine.deleteGradient()

                        textField.text = colorEditor.color
                        colorEditor.resetShapeColor()
                    }
                    tooltip: qsTr("Solid Color")
                }
                ButtonRowButton {
                    visible: colorEditor.supportGradient
                    iconSource: "images/icon_color_gradient.png"
                    onClicked: {
                        colorEditor.resetShapeColor()

                        if (colorEditor.shapeGradients)
                            gradientLine.gradientTypeName = "LinearGradient"
                        else
                            gradientLine.gradientTypeName = "Gradient"

                        if (gradientLine.hasGradient)
                            gradientLine.updateGradient()
                        else {
                            gradientLine.deleteGradient()
                            gradientLine.addGradient()
                        }
                    }

                    tooltip: qsTr("Linear Gradient")

                    GradientPopupIndicator {

                        onClicked: gradientDialogPopupLinear.toggle()

                        GradientDialogPopup {
                            id: gradientDialogPopupLinear

                            dialogHeight: 80
                            content: GridLayout {
                                rowSpacing: 4
                                anchors.fill: parent
                                height: 40

                                columns: 4
                                rows: 2

                                anchors.leftMargin: 12
                                anchors.rightMargin: 6

                                anchors.topMargin: 28
                                anchors.bottomMargin: 6

                                Label {
                                    text: "X1"
                                    width: 18
                                    tooltip: qsTr("Defines the start point for color interpolation.")
                                }

                                GradientPropertySpinBox {
                                    propertyName: "x1"
                                }

                                Label {
                                    text: "X2"
                                    width: 18
                                    tooltip: qsTr("Defines the end point for color interpolation.")
                                }

                                GradientPropertySpinBox {
                                    propertyName: "x2"
                                }

                                Label {
                                    text: "y1"
                                    width: 18
                                    tooltip: qsTr("Defines the start point for color interpolation.")
                                }

                                GradientPropertySpinBox {
                                    propertyName: "y1"
                                }

                                Label {
                                    text: "Y2"
                                    width: 18
                                    tooltip: qsTr("Defines the end point for color interpolation.")
                                }

                                GradientPropertySpinBox {
                                    propertyName: "y2"
                                }
                            }
                        }
                    }
                }
                ButtonRowButton {
                    visible: colorEditor.supportGradient && colorEditor.shapeGradients
                    iconSource: "images/icon_color_radial_gradient.png"
                    onClicked: {
                        colorEditor.resetShapeColor()
                        gradientLine.gradientTypeName = "RadialGradient"

                        if (gradientLine.hasGradient)
                            gradientLine.updateGradient()
                        else {
                            gradientLine.deleteGradient()
                            gradientLine.addGradient()
                        }
                    }

                    tooltip: qsTr("Radial Gradient")

                    GradientPopupIndicator {
                        onClicked: gradientDialogPopupRadial.toggle()

                        GradientDialogPopup {
                            id: gradientDialogPopupRadial
                            dialogHeight: 140
                            content: GridLayout {
                                rowSpacing: 4
                                anchors.fill: parent
                                height: 40

                                columns: 4
                                rows: 3

                                anchors.leftMargin: 12
                                anchors.rightMargin: 6

                                anchors.topMargin: 28
                                anchors.bottomMargin: 6

                                Label {
                                    text: "CenterX"
                                    width: 64
                                    tooltip: qsTr("Defines the center point.")
                                }

                                GradientPropertySpinBox {
                                    propertyName: "centerX"
                                }

                                Label {
                                    text: "CenterY"
                                    width: 64
                                    tooltip: qsTr("Defines the center point.")
                                }

                                GradientPropertySpinBox {
                                    propertyName: "centerY"
                                }

                                Label {
                                    text: "FocalX"
                                    width: 64
                                    tooltip: qsTr("Defines the focal point.")
                                }

                                GradientPropertySpinBox {
                                    propertyName: "focalX"
                                }

                                Label {
                                    text: "FocalY"
                                    width: 64
                                    tooltip: qsTr("Defines the focal point.")
                                }

                                GradientPropertySpinBox {
                                    propertyName: "focalY"
                                }

                                Label {
                                    text: "Center Radius"
                                    width: 64
                                    tooltip: qsTr("Defines the center point.")
                                }

                                GradientPropertySpinBox {
                                    propertyName: "centerRadius"
                                }

                                Label {
                                    text: "Focal Radius"
                                    width: 64
                                    tooltip: qsTr("Defines the focal radius. Set to 0 for simple radial gradients.")
                                }

                                GradientPropertySpinBox {
                                    propertyName: "focalRadius"
                                }
                            }
                        }
                    }
                }
                ButtonRowButton {
                    visible: colorEditor.supportGradient && colorEditor.shapeGradients
                    iconSource: "images/icon_color_conical_gradient.png"
                    onClicked: {
                        colorEditor.resetShapeColor()
                        gradientLine.gradientTypeName = "ConicalGradient"

                        if (gradientLine.hasGradient)
                            gradientLine.updateGradient()
                        else {
                            gradientLine.deleteGradient()
                            gradientLine.addGradient()
                        }
                    }

                    tooltip: qsTr("Conical Gradient")

                    GradientPopupIndicator {

                        onClicked: gradientDialogPopupConical.toggle()

                        GradientDialogPopup {
                            id: gradientDialogPopupConical
                            dialogHeight: 80
                            content: GridLayout {
                                rowSpacing: 4
                                anchors.fill: parent
                                height: 40

                                columns: 4
                                rows: 2

                                anchors.leftMargin: 12
                                anchors.rightMargin: 6

                                anchors.topMargin: 28
                                anchors.bottomMargin: 6

                                Label {
                                    text: "CenterX"
                                    width: 64
                                    tooltip: qsTr("Defines the center point.")
                                }

                                GradientPropertySpinBox {
                                    propertyName: "centerX"
                                }

                                Label {
                                    text: "CenterY"
                                    width: 64
                                    tooltip: qsTr("Defines the center point.")
                                }

                                GradientPropertySpinBox {
                                    propertyName: "centerY"
                                }

                                Label {
                                    text: "Angle"
                                    width: 64
                                    tooltip: qsTr("Defines the start angle for the conical gradient. The value is in degrees (0-360).")
                                }

                                GradientPropertySpinBox {
                                    propertyName: "angle"
                                }
                            }
                        }
                    }
                }
                ButtonRowButton {
                    id: transparentButton
                    iconSource: "images/icon_color_none.png"
                    onClicked: {
                        gradientLine.deleteGradient()
                        colorEditor.resetShapeColor()
                        colorEditor.color = "#00000000"
                    }
                    tooltip: qsTr("Transparent")
                }
            }

            Rectangle {
                id: gradientPickerButton
                width: 20
                height: 20
                visible: colorEditor.supportGradient

                color: Theme.qmlDesignerButtonColor()
                border.color: Theme.qmlDesignerBorderColor()
                border.width: 1

                ToolTipArea {
                    anchors.fill: parent
                    id: toolTipArea
                    tooltip: qsTr("Gradient Picker Dialog")
                }

                GradientPresetList {
                    id: presetList
                    visible: false

                    function applyPreset() {
                        if (!gradientLine.hasGradient)
                        {
                            if (colorEditor.shapeGradients)
                                gradientLine.gradientTypeName = "LinearGradient"
                            else
                                gradientLine.gradientTypeName = "Gradient"
                        }

                        if (presetList.gradientData.presetType == 0) {
                            gradientLine.setPresetByID(presetList.gradientData.presetID);
                        }
                        else if (presetList.gradientData.presetType == 1) {
                            gradientLine.setPresetByStops(
                                        presetList.gradientData.stops,
                                        presetList.gradientData.colors,
                                        presetList.gradientData.stopsCount);
                        }
                        else {
                            console.log("INVALID GRADIENT TYPE: " +
                                        presetList.gradientData.presetType);
                        }
                    }

                    onApply: {
                        if (presetList.gradientData.stopsCount > 0) {
                            applyPreset();
                        }
                    }

                    onSaved: {
                        gradientLine.savePreset();
                        presetList.updatePresets();
                    }

                    onAccepted: { //return key
                        if (presetList.gradientData.stopsCount > 0) {
                            applyPreset();
                        }
                    }
                }

                Image {
                    id: image
                    width: 16
                    height: 16
                    smooth: false
                    anchors.centerIn: parent
                    source: "images/icon-gradient-list.png"
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        presetList.open()
                    }
                }
            }

            ExpandingSpacer {
            }
        }

        Item {
            height: 8
        }

        ColorButton {
            property color bindedColor: colorEditor.color

            //prevent the binding to be deleted by assignment
            onBindedColorChanged: {
                colorButton.color = colorButton.bindedColor
            }

            enabled: !colorEditor.transparent
            opacity: checkButton.checked ? 1 : 0
            id: colorButton

            Layout.preferredWidth: 124
            Layout.preferredHeight: checkButton.checked ? 124 : 0

            sliderMargins: 4

            onUpdateColor: {
                colorEditor.color = colorButton.color
                if (contextMenu.opened)
                    contextMenu.close()
            }

            onRightMouseButtonClicked: contextMenu.popup(colorButton)
        }

        Item {
            height: 1
        }

        Item {
            height: 2
            visible: checkButton.checked
        }

        Item {
            height: 1
        }

        Item {
            id: colorBoxes

            Layout.preferredWidth: 134
            Layout.preferredHeight: checkButton.checked ? 70 : 0
            visible: checkButton.checked


            SecondColumnLayout {
                spacing: 16
                RowLayout {
                    spacing: 2
                    Column {
                        spacing: 5
                        Label {
                            width: parent.width
                            text: qsTr("Original")
                            color: "#eee"
                        }
                        Rectangle {
                            id: originalColorRectangle
                            color: colorEditor.originalColor
                            height: 40
                            width: 67

                            border.width: 1
                            border.color: "#555555"

                            ToolTipArea {
                                anchors.fill: parent

                                tooltip: originalColorRectangle.color
                                onClicked: {
                                    if (!colorEditor.transparent)
                                        colorEditor.color = colorEditor.originalColor
                                }
                            }
                        }
                    }

                    Column {
                        spacing: 5
                        Label {
                            width: parent.width
                            text: qsTr("New")
                            color: "#eee"
                        }
                        Rectangle {
                            id: newColorRectangle
                            color: colorEditor.color
                            height: 40
                            width: 67

                            border.width: 1
                            border.color: "#555555"
                        }
                    }
                }

                Column {
                    spacing: 5
                    Label {
                        width: parent.width
                        text: qsTr("Recent")
                        color: "#eee"
                        elide: Text.ElideRight
                    }

                    SimpleColorPalette {
                        id: colorPalette

                        clickable: !colorEditor.transparent

                        onSelectedColorChanged: colorEditor.color = colorPalette.selectedColor


                        onDialogColorChanged: colorEditor.color = colorPalette.selectedColor
                    }
                }
            }
        }
    }

    StudioControls.Menu {
        id: contextMenu

        StudioControls.MenuItem {
            text: qsTr("Open Color Dialog")
            onTriggered: colorPalette.showColorDialog(colorEditor.color)
        }
    }
}
