/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Shapes 1.15
import QtQuick.Templates 2.15 as T
import QtQuickDesignerTheme 1.0
import StudioTheme 1.0 as StudioTheme
import StudioControls 1.0 as StudioControls
import QtQuickDesignerColorPalette 1.0

SecondColumnLayout {
    id: colorEditor

    property color color
    property bool supportGradient: false
    property variant backendValue
    property variant value: {
        if (colorEditor.isVector3D)
            return Qt.rgba(colorEditor.backendValue.value.x,
                           colorEditor.backendValue.value.y,
                           colorEditor.backendValue.value.z,
                           1)
        else
            return colorEditor.backendValue.value
    }
    property alias gradientPropertyName: gradientLine.gradientPropertyName
    property bool shapeGradients: false
    property color originalColor
    property bool isVector3D: false

    function isNotInGradientMode() {
        return ceMode.currentValue === "Solid"
    }

    function hasLinearGradient() {
        return ceMode.currentValue === "LinearGradient"
    }

    function hasConicalGradient() {
        return ceMode.currentValue === "ConicalGradient"
    }

    function hasRadialGradient() {
        return ceMode.currentValue === "RadialGradient"
    }

    function resetShapeColor() {
        colorEditor.backendValue.resetValue()
    }

    function updateThumbnail() {
        if (!gradientLine.hasGradient)
            return

        if (!colorEditor.shapeGradients) {
            var gradientString = "import QtQuick 2.15; Gradient {"
            var orientation = gradientOrientation.currentValue === Gradient.Horizontal ? "Gradient.Horizontal"
                                                                                       : "Gradient.Vertical"
            gradientString += "orientation: " + orientation + ";"

            for (var i = 0; i < gradientLine.model.count; i++)
                gradientString += "GradientStop {}"

            gradientString += "}"

            var gradientObject = Qt.createQmlObject(gradientString, gradientThumbnail, "dynamicGradient")

            for (i = 0; i < gradientLine.model.count; i++) {
                gradientObject.stops[i].color = gradientLine.model.getColor(i)
                gradientObject.stops[i].position = gradientLine.model.getPosition(i)
            }

            gradientThumbnail.gradient = gradientObject
        } else {
            var gradientStr = "import QtQuick 2.15; import QtQuick.Shapes 1.15; "
                    + gradientLine.gradientTypeName + " {"

            if (gradientLine.gradientTypeName === "LinearGradient") {
                gradientStr += "x1: 0"
                            + ";x2: " + shape.width
                            + ";y1: 0"
                            + ";y2: " + shape.height + ";"
            } else if (gradientLine.gradientTypeName === "RadialGradient") {
                gradientStr += "centerX: " + shape.width * 0.5
                            + ";centerY: " + shape.height * 0.5
                            + ";focalX: " + shape.width * 0.5
                            + ";focalY: " + shape.height * 0.5
                            + ";centerRadius: " + Math.min(shape.width, shape.height) * 0.5
                            + ";focalRadius: 0" + ";"
            } else if (gradientLine.gradientTypeName === "ConicalGradient") {
                gradientStr += "centerX: " + shape.width * 0.5
                            + ";centerY: " + shape.height * 0.5
                            + ";angle: 0" + ";"
            }

            for (var j = 0; j < gradientLine.model.count; j++)
                gradientStr += "GradientStop {}"

            gradientStr += "}"

            var gradientObj = Qt.createQmlObject(gradientStr, shapeGradientThumbnail, "dynamicShapeGradient")

            for (j = 0; j < gradientLine.model.count; j++) {
                gradientObj.stops[j].color = gradientLine.model.getColor(j)
                gradientObj.stops[j].position = gradientLine.model.getPosition(j)
            }

            shapeGradientThumbnail.fillGradient = gradientObj
        }
    }

    function createModel() {
        // Build the color editor combobox model
        ceMode.items.clear()
        ceMode.items.append({
            value: "Solid",
            text: qsTr("Solid"),
            test: true
        })
        ceMode.items.append({
            value: "LinearGradient",
            text: qsTr("Linear"),
            test: colorEditor.supportGradient
        })
        ceMode.items.append({
            value: "RadialGradient",
            text: qsTr("Radial"),
            test: colorEditor.supportGradient && colorEditor.shapeGradients
        })
        ceMode.items.append({
            value: "ConicalGradient",
            text: qsTr("Conical"),
            test: colorEditor.supportGradient && colorEditor.shapeGradients
        })
    }

    function determineActiveColorMode() {
        if (colorEditor.supportGradient && gradientLine.hasGradient) {
            if (colorEditor.shapeGradients) {
                switch (gradientLine.gradientTypeName) {
                case "LinearGradient":
                    ceMode.currentIndex = ceMode.indexOfValue("LinearGradient")
                    break
                case "RadialGradient":
                    ceMode.currentIndex = ceMode.indexOfValue("RadialGradient")
                    break
                case "ConicalGradient":
                    ceMode.currentIndex = ceMode.indexOfValue("ConicalGradient")
                    break
                default:
                    ceMode.currentIndex = ceMode.indexOfValue("LinearGradient")
                }
            } else {
                ceMode.currentIndex = ceMode.indexOfValue("LinearGradient")
            }
            colorEditor.color = gradientLine.currentColor
        } else {
            ceMode.currentIndex = ceMode.indexOfValue("Solid")
            colorEditor.color = colorEditor.value
        }

        colorEditor.originalColor = colorEditor.color
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
                                                                 colorEditor.color.b)
                } else {
                    colorEditor.backendValue.value = colorEditor.color
                }
            }
        }
    }

    onColorChanged: {
        if (!gradientLine.isInValidState)
            return

        if (colorEditor.supportGradient && gradientLine.hasGradient) {
            var hexColor = convertColorToString(colorEditor.color)
            hexTextField.text = hexColor
            popupHexTextField.text = hexColor
            gradientLine.currentColor = colorEditor.color
        }

        if (isNotInGradientMode())
            colorEditorTimer.restart() // Delay setting the color to keep ui responsive

        colorPalette.selectedColor = colorEditor.color
    }

    Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

    Rectangle {
        id: preview
        implicitWidth: StudioTheme.Values.twoControlColumnWidth
        implicitHeight: StudioTheme.Values.height
        color: colorEditor.color
        border.color: StudioTheme.Values.themeControlOutline
        border.width: StudioTheme.Values.border

        Rectangle {
            id: gradientThumbnail
            anchors.fill: parent
            anchors.margins: StudioTheme.Values.border
            visible: !colorEditor.isNotInGradientMode()
                     && !colorEditor.shapeGradients
                     && colorEditor.hasLinearGradient()
        }

        Shape {
            id: shape
            anchors.fill: parent
            anchors.margins: StudioTheme.Values.border
            visible: !colorEditor.isNotInGradientMode()
                     && colorEditor.shapeGradients

            ShapePath {
                id: shapeGradientThumbnail
                startX: shape.x - 1
                startY: shape.y - 1
                strokeWidth: -1
                strokeColor: "green"

                PathLine { x: shape.x - 1; y: shape.height }
                PathLine { x: shape.width; y: shape.height }
                PathLine { x: shape.width; y: shape.y - 1 }
            }
        }

        Image {
            anchors.fill: parent
            source: "images/checkers.png"
            fillMode: Image.Tile
            z: -1
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                cePopup.opened ? cePopup.close() : cePopup.open()
                forceActiveFocus()
            }
        }

        T.Popup {
            id: cePopup

            WheelHandler {
                onWheel: function(event) {
                    Controller.mainScrollView.flick(0, event.angleDelta.y * 5)
                }
            }

            onOpened: {
                if (Controller.mainScrollView === null)
                    return

                var mapped = preview.mapToItem(Controller.mainScrollView.contentItem, cePopup.x, cePopup.y)
                Controller.mainScrollView.temporaryHeight = mapped.y + cePopup.height + 20
            }

            onHeightChanged: {
                if (Controller.mainScrollView === null)
                    return

                var mapped = preview.mapToItem(Controller.mainScrollView.contentItem, cePopup.x, cePopup.y)
                Controller.mainScrollView.temporaryHeight = mapped.y + cePopup.height + 20
            }

            onClosed: {
                Controller.mainScrollView.temporaryHeight = 0
            }

            x: - StudioTheme.Values.colorEditorPopupWidth * 0.5
               + preview.width * 0.5
            y: - StudioTheme.Values.colorEditorPopupMargin
               - (StudioTheme.Values.colorEditorPopupSpacing * 2)
               - StudioTheme.Values.defaultControlHeight
               - StudioTheme.Values.colorEditorPopupLineHeight
               - colorPicker.height * 0.5
               + preview.height * 0.5

            width: StudioTheme.Values.colorEditorPopupWidth
            height: colorColumn.height + sectionColumn.height
                    + StudioTheme.Values.colorEditorPopupMargin + 2 // TODO magic number

            padding: StudioTheme.Values.border
            margins: -1 // If not defined margin will be -1

            closePolicy: T.Popup.CloseOnPressOutside | T.Popup.CloseOnPressOutsideParent

            contentItem: Item {
                id: todoItem

                property color color
                property bool supportGradient: false

                Column {
                    id: colorColumn

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: StudioTheme.Values.colorEditorPopupMargin
                    spacing: StudioTheme.Values.colorEditorPopupSpacing

                    RowLayout {
                        width: parent.width
                        Layout.alignment: Qt.AlignTop

                        StudioControls.ComboBox {
                            id: ceMode

                            property ListModel items: ListModel {}

                            implicitWidth: StudioTheme.Values.colorEditorPopupCmoboBoxWidth
                            width: implicitWidth
                            actionIndicatorVisible: false
                            textRole: "text"
                            valueRole: "value"
                            model: ceMode.items
                            onActivated: {
                                switch (ceMode.currentValue) {
                                case "Solid":
                                    gradientLine.deleteGradient()
                                    hexTextField.text = colorEditor.color
                                    popupHexTextField.text = colorEditor.color
                                    colorEditor.resetShapeColor()
                                    break
                                case "LinearGradient":
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
                                    break
                                case "RadialGradient":
                                    colorEditor.resetShapeColor()
                                    gradientLine.gradientTypeName = "RadialGradient"

                                    if (gradientLine.hasGradient)
                                        gradientLine.updateGradient()
                                    else {
                                        gradientLine.deleteGradient()
                                        gradientLine.addGradient()
                                    }
                                    break
                                case "ConicalGradient":
                                    colorEditor.resetShapeColor()
                                    gradientLine.gradientTypeName = "ConicalGradient"

                                    if (gradientLine.hasGradient)
                                        gradientLine.updateGradient()
                                    else {
                                        gradientLine.deleteGradient()
                                        gradientLine.addGradient()
                                    }
                                    break
                                default:
                                    console.log("Unknown item selected in color mode ComboBox.")
                                }
                                colorEditor.updateThumbnail()
                            }
                        }

                        ExpandingSpacer {}

                        IconIndicator {
                            id: transparentIndicator
                            icon: StudioTheme.Constants.transparent
                            pixelSize: StudioTheme.Values.myIconFontSize * 1.4
                            tooltip: qsTr("Transparent TODO")
                            onClicked: {
                                colorPicker.alpha = 0
                                colorPicker.updateColor()
                            }
                        }

                        IconIndicator {
                            id: gradientPickerIndicator
                            icon: StudioTheme.Constants.gradient
                            pixelSize: StudioTheme.Values.myIconFontSize * 1.4
                            tooltip: qsTr("Gradient Picker")
                            enabled: colorEditor.supportGradient
                            onClicked: presetList.show()

                            GradientPresetList {
                                id: presetList
                                visible: false

                                function applyPreset() {
                                    if (!gradientLine.hasGradient) {
                                        if (colorEditor.shapeGradients)
                                            gradientLine.gradientTypeName = "LinearGradient"
                                        else
                                            gradientLine.gradientTypeName = "Gradient"
                                    }

                                    if (presetList.gradientData.presetType == 0) {
                                        gradientLine.setPresetByID(presetList.gradientData.presetID)
                                    } else if (presetList.gradientData.presetType == 1) {
                                        gradientLine.setPresetByStops(
                                                    presetList.gradientData.stops,
                                                    presetList.gradientData.colors,
                                                    presetList.gradientData.stopsCount)
                                    } else {
                                        console.log("INVALID GRADIENT TYPE: " +
                                                    presetList.gradientData.presetType)
                                    }
                                }

                                onApplied: {
                                    if (presetList.gradientData.stopsCount > 0)
                                        applyPreset()
                                }

                                onSaved: {
                                    gradientLine.savePreset()
                                    presetList.updatePresets()
                                }

                                onAccepted: { // return key
                                    if (presetList.gradientData.stopsCount > 0)
                                        applyPreset()
                                }
                            }
                        }

                        IconIndicator {
                            id: eyeDropperIndicator
                            icon: StudioTheme.Constants.eyeDropper
                            pixelSize: StudioTheme.Values.myIconFontSize * 1.4
                            tooltip: qsTr("Eye Dropper")
                            onClicked: ColorPaletteSingleton.eyeDropper()
                        }

                        IconIndicator {
                            id: closeIndicator
                            icon: StudioTheme.Constants.colorPopupClose
                            pixelSize: StudioTheme.Values.myIconFontSize * 1.4
                            onClicked: cePopup.close()
                        }
                    }

                    ColorLine {
                        id: colorLine
                        width: parent.width
                        currentColor: colorEditor.color
                        visible: isNotInGradientMode()
                    }

                    GradientLine {
                        id: gradientLine
                        property bool isInValidState: false
                        width: parent.width
                        visible: !isNotInGradientMode()

                        onCurrentColorChanged: {
                            if (colorEditor.supportGradient && gradientLine.hasGradient)
                                colorEditor.color = gradientLine.currentColor
                        }

                        onHasGradientChanged: {
                            if (!colorEditor.supportGradient)
                                return

                            colorEditor.determineActiveColorMode()
                        }

                        onSelectedNodeChanged: {
                            if (colorEditor.supportGradient && gradientLine.hasGradient) {
                                colorEditor.originalColor = gradientLine.currentColor
                            }
                        }

                        onInvalidated: colorEditor.updateThumbnail()

                        Connections {
                            target: modelNodeBackend
                            function onSelectionToBeChanged() {
                                colorEditorTimer.stop()
                                gradientLine.isInValidState = false

                                var hexOriginalColor = convertColorToString(colorEditor.originalColor)
                                var hexColor = convertColorToString(colorEditor.color)

                                if (hexOriginalColor !== hexColor) {
                                    if (colorEditor.color !== "#ffffff"
                                        && colorEditor.color !== "#000000"
                                        && colorEditor.color !== "#00000000") {
                                        colorPalette.addColorToPalette(colorEditor.color)
                                    }
                                }
                            }
                        }

                        Connections {
                            target: modelNodeBackend
                            function onSelectionChanged() {
                                if (colorEditor.supportGradient && gradientLine.hasGradient) {
                                    colorEditor.color = gradientLine.currentColor
                                    gradientLine.currentColor = color
                                    hexTextField.text = colorEditor.color
                                    popupHexTextField.text = colorEditor.color
                                }
                                gradientLine.isInValidState = true
                                colorEditor.originalColor = colorEditor.color
                                colorPalette.selectedColor = colorEditor.color

                                colorEditor.createModel()
                                colorEditor.determineActiveColorMode()
                            }
                        }
                    }

                    ColorPicker {
                        id: colorPicker

                        property color boundColor: colorEditor.color

                        width: parent.width
                        sliderMargins: 4

                        // Prevent the binding to be deleted by assignment
                        onBoundColorChanged: colorPicker.color = colorPicker.boundColor
                        onUpdateColor: {
                            colorEditor.color = colorPicker.color
                            if (contextMenu.opened)
                                contextMenu.close()
                        }
                        onRightMouseButtonClicked: contextMenu.popup(colorPicker)

                        onColorInvalidated: {
                            if (colorPicker.saturation > 0.0 && colorPicker.lightness > 0.0) {
                                hueSpinBox.value = colorPicker.hue
                            }

                            if (colorPicker.lightness > 0.0)
                                saturationSpinBox.value = colorPicker.saturation
                            else
                                colorPicker.saturation = saturationSpinBox.value

                            lightnessSpinBox.value = colorPicker.lightness
                            hslaAlphaSpinBox.value = colorPicker.alpha

                            redSpinBox.value = (colorPicker.color.r * 255)
                            greenSpinBox.value = (colorPicker.color.g * 255)
                            blueSpinBox.value = (colorPicker.color.b * 255)
                            rgbaAlphaSpinBox.value = (colorPicker.alpha * 255)
                        }
                    }

                    Column {
                        id: colorCompare
                        width: parent.width

                        RowLayout {
                            width: parent.width
                            Layout.alignment: Qt.AlignTop
                            spacing: StudioTheme.Values.controlGap

                            Label {
                                text: qsTr("Original")
                                width: 2 * StudioTheme.Values.colorEditorPopupSpinBoxWidth
                                       + StudioTheme.Values.controlGap
                            }

                            Label {
                                text: qsTr("New")
                                width: 2 * StudioTheme.Values.colorEditorPopupSpinBoxWidth
                                       + StudioTheme.Values.controlGap
                            }
                        }

                        RowLayout {
                            width: parent.width
                            Layout.alignment: Qt.AlignTop
                            spacing: StudioTheme.Values.controlGap

                            Rectangle {
                                id: originalColorRectangle
                                color: colorEditor.originalColor
                                height: StudioTheme.Values.height
                                width: 2 * StudioTheme.Values.colorEditorPopupSpinBoxWidth
                                       + StudioTheme.Values.controlGap
                                border.width: StudioTheme.Values.border
                                border.color: StudioTheme.Values.themeControlOutline

                                Image {
                                    anchors.fill: parent
                                    source: "images/checkers.png"
                                    fillMode: Image.Tile
                                    z: -1
                                }

                                ToolTipArea {
                                    anchors.fill: parent
                                    tooltip: originalColorRectangle.color
                                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                                    onClicked: function(mouse) {
                                        if (mouse.button === Qt.LeftButton)
                                            colorEditor.color = colorEditor.originalColor

                                        if (mouse.button === Qt.RightButton) {
                                            contextMenuFavorite.currentColor = colorEditor.originalColor
                                            contextMenuFavorite.popup()
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                id: newColorRectangle
                                color: colorEditor.color
                                height: StudioTheme.Values.height
                                width: 2 * StudioTheme.Values.colorEditorPopupSpinBoxWidth
                                       + StudioTheme.Values.controlGap
                                border.width: StudioTheme.Values.border
                                border.color: StudioTheme.Values.themeControlOutline

                                Image {
                                    anchors.fill: parent
                                    source: "images/checkers.png"
                                    fillMode: Image.Tile
                                    z: -1
                                }

                                ToolTipArea {
                                    anchors.fill: parent
                                    tooltip: newColorRectangle.color
                                    acceptedButtons: Qt.RightButton
                                    onClicked: function(mouse) {
                                        if (mouse.button === Qt.RightButton) {
                                            contextMenuFavorite.currentColor = colorEditor.color
                                            contextMenuFavorite.popup()
                                        }
                                    }
                                }
                            }
                        }

                        StudioControls.Menu {
                            id: contextMenuFavorite

                            property color currentColor

                            StudioControls.MenuItem {
                                text: qsTr("Add to Favorites")
                                onTriggered: ColorPaletteSingleton.addFavoriteColor(
                                                 contextMenuFavorite.currentColor)
                            }
                        }
                    }
                }

                Column {
                    id: sectionColumn
                    anchors.topMargin: StudioTheme.Values.colorEditorPopupMargin
                    anchors.top: colorColumn.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right

                    bottomPadding: 10

                    Section {
                        caption: qsTr("Color Details")
                        anchors.left: parent.left
                        anchors.right: parent.right

                        leftPadding: 10
                        rightPadding: 10

                        Column {
                            spacing: 10

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                LineEdit {
                                    id: popupHexTextField
                                    implicitWidth: 2 * StudioTheme.Values.colorEditorPopupSpinBoxWidth
                                                   + StudioTheme.Values.controlGap
                                    width: implicitWidth
                                    writeValueManually: true
                                    validator: RegExpValidator { regExp: /#[0-9A-Fa-f]{6}([0-9A-Fa-f]{2})?/g }
                                    showTranslateCheckBox: false
                                    showExtendedFunctionButton: false
                                    backendValue: colorEditor.backendValue

                                    onAccepted: colorEditor.color = colorFromString(popupHexTextField.text)
                                    onCommitData: {
                                        colorEditor.color = colorFromString(popupHexTextField.text)
                                        if (isNotInGradientMode()) {
                                            if (colorEditor.isVector3D) {
                                                backendValue.value = Qt.vector3d(colorEditor.color.r,
                                                                                 colorEditor.color.g,
                                                                                 colorEditor.color.b)
                                            } else {
                                                backendValue.value = colorEditor.color
                                            }
                                        }
                                    }
                                }

                                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                                ControlLabel {
                                    text: "Hex"
                                    width: StudioTheme.Values.colorEditorPopupHexLabelWidth
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                StudioControls.ComboBox {
                                    id: colorMode

                                    implicitWidth: 3 * StudioTheme.Values.controlGap
                                                   + 4 * StudioTheme.Values.colorEditorPopupSpinBoxWidth
                                    width: implicitWidth
                                    actionIndicatorVisible: false
                                    model: ["RGBA", "HSLA"]
                                    onActivated: {
                                        switch (colorMode.currentText) {
                                        case "RGBA":
                                            rgbaRow.visible = true
                                            hslaRow.visible = false
                                            break
                                        case "HSLA":
                                            rgbaRow.visible = false
                                            hslaRow.visible = true
                                            break
                                        default:
                                            console.log("Unknown color mode selected.")
                                            rgbaRow.visible = true
                                            hslaRow.visible = false
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                id: rgbaRow

                                Layout.fillWidth: true
                                spacing: StudioTheme.Values.controlGap

                                DoubleSpinBox {
                                    id: redSpinBox
                                    width: StudioTheme.Values.colorEditorPopupSpinBoxWidth

                                    stepSize: 1
                                    minimumValue: 0
                                    maximumValue: 255
                                    decimals: 0

                                    onValueModified: {
                                        var tmp = redSpinBox.value / 255.0
                                        if (colorPicker.color.r !== tmp && !colorPicker.block) {
                                            colorPicker.color.r = tmp
                                            colorPicker.updateColor()
                                        }
                                    }
                                }

                                DoubleSpinBox {
                                    id: greenSpinBox
                                    width: StudioTheme.Values.colorEditorPopupSpinBoxWidth

                                    stepSize: 1
                                    minimumValue: 0
                                    maximumValue: 255
                                    decimals: 0

                                    onValueModified: {
                                        var tmp = greenSpinBox.value / 255.0
                                        if (colorPicker.color.g !== tmp && !colorPicker.block) {
                                            colorPicker.color.g = tmp
                                            colorPicker.updateColor()
                                        }
                                    }
                                }

                                DoubleSpinBox {
                                    id: blueSpinBox
                                    width: StudioTheme.Values.colorEditorPopupSpinBoxWidth

                                    stepSize: 1
                                    minimumValue: 0
                                    maximumValue: 255
                                    decimals: 0

                                    onValueModified: {
                                        var tmp = blueSpinBox.value / 255.0
                                        if (colorPicker.color.b !== tmp && !colorPicker.block) {
                                            colorPicker.color.b = tmp
                                            colorPicker.updateColor()
                                        }
                                    }
                                }

                                DoubleSpinBox {
                                    id: rgbaAlphaSpinBox
                                    width: StudioTheme.Values.colorEditorPopupSpinBoxWidth

                                    stepSize: 1
                                    minimumValue: 0
                                    maximumValue: 255
                                    decimals: 0

                                    onValueModified: {
                                        var tmp = rgbaAlphaSpinBox.value / 255.0
                                        if (colorPicker.alpha !== tmp && !colorPicker.block) {
                                            colorPicker.alpha = tmp
                                            colorPicker.updateColor()
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                id: hslaRow

                                visible: false
                                Layout.fillWidth: true
                                spacing: StudioTheme.Values.controlGap

                                DoubleSpinBox {
                                    id: hueSpinBox
                                    width: StudioTheme.Values.colorEditorPopupSpinBoxWidth
                                    onValueModified: {
                                        if (colorPicker.hue !== hueSpinBox.value && !colorPicker.block) {
                                            colorPicker.hue = hueSpinBox.value
                                            colorPicker.updateColor()
                                        }
                                    }
                                }

                                DoubleSpinBox {
                                    id: saturationSpinBox
                                    width: StudioTheme.Values.colorEditorPopupSpinBoxWidth
                                    onValueModified: {
                                        if (colorPicker.saturation !== saturationSpinBox.value && !colorPicker.block) {
                                            colorPicker.saturation = saturationSpinBox.value
                                            colorPicker.updateColor()
                                        }
                                    }
                                }

                                DoubleSpinBox {
                                    id: lightnessSpinBox
                                    width: StudioTheme.Values.colorEditorPopupSpinBoxWidth
                                    onValueModified: {
                                        if (colorPicker.lightness !== lightnessSpinBox.value && !colorPicker.block) {
                                            colorPicker.lightness = lightnessSpinBox.value
                                            colorPicker.updateColor()
                                        }
                                    }
                                }

                                DoubleSpinBox {
                                    id: hslaAlphaSpinBox
                                    width: StudioTheme.Values.colorEditorPopupSpinBoxWidth
                                    onValueModified: {
                                        if (colorPicker.alpha !== hslaAlphaSpinBox.value && !colorPicker.block) {
                                            colorPicker.alpha = hslaAlphaSpinBox.value
                                            colorPicker.updateColor()
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Section {
                        caption: qsTr("Palette")
                        anchors.left: parent.left
                        anchors.right: parent.right

                        leftPadding: 10
                        rightPadding: 10
                        bottomPadding: 5

                        ColorPalette {
                            id: colorPalette
                            enableSingletonConnection: cePopup.opened
                            onSelectedColorChanged: colorEditor.color = colorPalette.selectedColor
                            onDialogColorChanged: colorEditor.color = colorPalette.selectedColor
                        }
                    }

                    Section {
                        id: gradientControls
                        caption: qsTr("Gradient Controls")
                        anchors.left: parent.left
                        anchors.right: parent.right
                        visible: !colorEditor.isNotInGradientMode()

                        leftPadding: 10
                        rightPadding: 10

                        component ControlsRow: RowLayout {
                            property alias propertyName: spinBox.propertyName
                            property alias labelText: label.text
                            property alias labelTooltip: label.tooltip
                            property alias value: spinBox.value

                            Layout.fillWidth: true
                            spacing: 0

                            GradientPropertySpinBox {
                                id: spinBox
                                implicitWidth: StudioTheme.Values.controlGap
                                               + 2 * StudioTheme.Values.colorEditorPopupSpinBoxWidth
                                width: implicitWidth
                            }

                            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                            ControlLabel {
                                id: label
                                horizontalAlignment: Text.AlignLeft
                                width: StudioTheme.Values.controlGap
                                       + 2 * StudioTheme.Values.colorEditorPopupSpinBoxWidth
                            }
                        }

                        // Default Gradient Controls
                        Column {
                            id: defaultGradientControls
                            spacing: 10

                            visible: colorEditor.hasLinearGradient() && !colorEditor.shapeGradients

                            RowLayout {
                                id: defaultGradientOrientation

                                Layout.fillWidth: true
                                spacing: 0

                                StudioControls.ComboBox {
                                    id: gradientOrientation
                                    implicitWidth: StudioTheme.Values.controlGap
                                                   + 3 * StudioTheme.Values.colorEditorPopupSpinBoxWidth
                                    width: implicitWidth
                                    model: [{ value: Gradient.Vertical, text: qsTr("Vertical") },
                                            { value: Gradient.Horizontal, text: qsTr("Horizontal") }]

                                    textRole: "text"
                                    valueRole: "value"
                                    onActivated: {
                                        gradientLine.model.setGradientOrientation(gradientOrientation.currentValue)
                                        colorEditor.updateThumbnail()
                                    }

                                    Component.onCompleted: {
                                        var orientation = gradientLine.model.readGradientOrientation()

                                        if (orientation === "Horizontal")
                                            gradientOrientation.currentIndex =
                                                    gradientOrientation.indexOfValue(Gradient.Horizontal)
                                        else
                                            gradientOrientation.currentIndex =
                                                    gradientOrientation.indexOfValue(Gradient.Vertical)
                                    }
                                }

                                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap + 6 }

                                IconLabel {
                                    id: iconLabel
                                    icon: StudioTheme.Constants.orientation
                                    pixelSize: StudioTheme.Values.myIconFontSize * 1.4
                                    tooltip: qsTr("Defines the direction of the gradient.")
                                }
                            }
                        }

                        // Linear Gradient Controls
                        Column {
                            id: linearGradientControls
                            spacing: 10

                            visible: colorEditor.hasLinearGradient() && colorEditor.shapeGradients

                            ControlsRow {
                                id: linearGradientX1
                                propertyName: "x1"
                                labelText: "X1"
                                labelTooltip: qsTr("Defines the start point for color interpolation.")
                            }

                            ControlsRow {
                                id: linearGradientX2
                                propertyName: "x2"
                                labelText: "X2"
                                labelTooltip: qsTr("Defines the end point for color interpolation.")
                            }

                            ControlsRow {
                                id: linearGradientY1
                                propertyName: "y1"
                                labelText: "Y1"
                                labelTooltip: qsTr("Defines the start point for color interpolation.")
                            }

                            ControlsRow {
                                id: linearGradientY2
                                propertyName: "y2"
                                labelText: "Y2"
                                labelTooltip: qsTr("Defines the end point for color interpolation.")
                            }
                        }

                        // Radial Gradient Controls
                        Column {
                            id: radialGradientControls
                            spacing: 10

                            visible: colorEditor.hasRadialGradient()

                            ControlsRow {
                                propertyName: "centerX"
                                labelText: "CenterX"
                                labelTooltip: qsTr("Defines the center point.")
                            }

                            ControlsRow {
                                propertyName: "centerY"
                                labelText: "CenterY"
                                labelTooltip: qsTr("Defines the center point.")
                            }

                            ControlsRow {
                                propertyName: "focalX"
                                labelText: "FocalX"
                                labelTooltip: qsTr("Defines the focal point.")
                            }

                            ControlsRow {
                                propertyName: "focalY"
                                labelText: "FocalY"
                                labelTooltip: qsTr("Defines the focal point.")
                            }

                            ControlsRow {
                                propertyName: "centerRadius"
                                labelText: "Center Radius"
                                labelTooltip: qsTr("Defines the center radius.")
                            }

                            ControlsRow {
                                propertyName: "focalRadius"
                                labelText: "Focal Radius"
                                labelTooltip: qsTr("Defines the focal radius. Set to 0 for simple radial gradients.")
                            }
                        }

                        // Conical Gradient Controls
                        Column {
                            id: concialGradientControls
                            spacing: 10

                            visible: colorEditor.hasConicalGradient()

                            ControlsRow {
                                propertyName: "centerX"
                                labelText: "CenterX"
                                labelTooltip: qsTr("Defines the center point.")
                            }

                            ControlsRow {
                                propertyName: "centerY"
                                labelText: "CenterY"
                                labelTooltip: qsTr("Defines the center point.")
                            }

                            ControlsRow {
                                propertyName: "angle"
                                labelText: "Angle"
                                labelTooltip: qsTr("Defines the start angle for the conical gradient. The value is in degrees (0-360).")
                            }
                        }
                    }
                }
            }

            background: Rectangle {
                color: StudioTheme.Values.themeControlBackground
                border.color: StudioTheme.Values.themeInteraction
                border.width: StudioTheme.Values.border
            }

            enter: Transition {}
            exit: Transition {}
        }
    }

    Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

    LineEdit {
        id: hexTextField
        implicitWidth: StudioTheme.Values.twoControlColumnWidth
                       + StudioTheme.Values.actionIndicatorWidth
        width: implicitWidth
        enabled: colorEditor.isNotInGradientMode()
        writeValueManually: true
        validator: RegExpValidator { regExp: /#[0-9A-Fa-f]{6}([0-9A-Fa-f]{2})?/g }
        showTranslateCheckBox: false
        backendValue: colorEditor.backendValue

        onAccepted: colorEditor.color = colorFromString(hexTextField.text)
        onCommitData: {
            colorEditor.color = colorFromString(hexTextField.text)
            if (isNotInGradientMode()) {
                if (colorEditor.isVector3D) {
                    backendValue.value = Qt.vector3d(colorEditor.color.r,
                                                     colorEditor.color.g,
                                                     colorEditor.color.b)
                } else {
                    backendValue.value = colorEditor.color
                }
            }
        }
    }

    Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

    ControlLabel {
        text: "Hex"
        horizontalAlignment: Text.AlignLeft
        width: StudioTheme.Values.controlLabelWidth
               + StudioTheme.Values.controlGap
               + StudioTheme.Values.linkControlWidth
    }

    ExpandingSpacer {}

    StudioControls.Menu {
        id: contextMenu

        StudioControls.MenuItem {
            text: qsTr("Open Color Dialog")
            onTriggered: colorPalette.showColorDialog(colorEditor.color)
        }
    }

    Component.onCompleted: colorEditor.determineActiveColorMode()
}
