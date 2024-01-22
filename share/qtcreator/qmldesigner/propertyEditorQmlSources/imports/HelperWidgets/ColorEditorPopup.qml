// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuick.Shapes
import QtQuick.Templates as T
import StudioTheme as StudioTheme
import StudioControls as StudioControls
import QtQuickDesignerColorPalette

Column {
    id: root

    property bool eyeDropperActive: ColorPaletteBackend.eyeDropperActive

    property bool supportGradient: false
    property bool shapeGradients: false
    property alias gradientLine: gradientLine
    property alias popupHexTextField: popupHexTextField
    property alias gradientPropertyName: root.gradientModel.gradientPropertyName
    property alias gradientOrientation: gradientOrientation

    property alias gradientModel: gradientModel

    property bool isInValidState: false

    readonly property real twoColumnWidth: (colorColumn.width - StudioTheme.Values.controlGap) * 0.5
    readonly property real fourColumnWidth: (colorColumn.width - (3 * StudioTheme.Values.controlGap)) * 0.25

    property Item titleBarContent: Row {
        anchors.fill: parent
        spacing: 10

        StudioControls.ComboBox {
            id: ceMode

            property ListModel items: ListModel {}

            anchors.verticalCenter: parent.verticalCenter
            enabled: isBaseState
            implicitWidth: StudioTheme.Values.colorEditorPopupComboBoxWidth
            width: ceMode.implicitWidth
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

                    if (root.shapeGradients)
                        gradientModel.gradientTypeName = "LinearGradient"
                    else
                        gradientModel.gradientTypeName = "Gradient"

                    if (gradientModel.hasGradient)
                        gradientLine.updateGradient()
                    else {
                        gradientLine.deleteGradient()
                        gradientLine.addGradient()
                    }
                    break
                case "RadialGradient":
                    colorEditor.resetShapeColor()
                    gradientModel.gradientTypeName = "RadialGradient"

                    if (gradientLine.hasGradient)
                        gradientLine.updateGradient()
                    else {
                        gradientLine.deleteGradient()
                        gradientLine.addGradient()
                    }
                    break
                case "ConicalGradient":
                    colorEditor.resetShapeColor()
                    gradientModel.gradientTypeName = "ConicalGradient"

                    if (gradientModel.hasGradient)
                        gradientLine.updateGradient()
                    else {
                        gradientLine.deleteGradient()
                        gradientLine.addGradient()
                    }
                    break
                default:
                    console.warn("Unknown item selected in color mode ComboBox.")
                }
                root.updateThumbnail()
            }

            ToolTipArea {
                enabled: !isBaseState
                anchors.fill: parent
                tooltip: qsTr("Fill type can only be changed in base state.")
                z: 10
            }
        }

        ExpandingSpacer {}

        IconIndicator {
            id: transparentIndicator
            anchors.verticalCenter: parent.verticalCenter
            icon: StudioTheme.Constants.transparent
            pixelSize: StudioTheme.Values.myIconFontSize * 1.4
            tooltip: qsTr("Transparent")
            onClicked: {
                colorPicker.alpha = 0
                colorPicker.invalidateColor()
                colorPicker.updateColor()
            }
        }

        IconIndicator {
            id: gradientPickerIndicator
            anchors.verticalCenter: parent.verticalCenter
            icon: StudioTheme.Constants.gradient
            pixelSize: StudioTheme.Values.myIconFontSize * 1.4
            tooltip: qsTr("Gradient Picker")
            enabled: root.supportGradient
            onClicked: presetList.show()

            GradientPresetList {
                id: presetList
                visible: false
                transientParent: root.parentWindow

                function applyPreset() {
                    if (!gradientModel.hasGradient) {
                        if (root.shapeGradients)
                            gradientModel.gradientTypeName = "LinearGradient"
                        else
                            gradientModel.gradientTypeName = "Gradient"
                    }

                    if (presetList.gradientData.presetType == 0) {
                        gradientLine.setPresetByID(presetList.gradientData.presetID)
                    } else if (presetList.gradientData.presetType == 1) {
                        gradientLine.setPresetByStops(
                                    presetList.gradientData.stops,
                                    presetList.gradientData.colors,
                                    presetList.gradientData.stopsCount)
                    } else {
                        console.warn("Invalid Gradient type:", presetList.gradientData.presetType)
                    }
                }

                onApplied: {
                    if (presetList.gradientData.stopsCount > 0)
                        presetList.applyPreset()
                }

                onSaved: {
                    gradientLine.savePreset()
                    presetList.updatePresets()
                }

                onAccepted: { // return key
                    if (presetList.gradientData.stopsCount > 0)
                        presetList.applyPreset()
                }
            }
        }

        IconIndicator {
            id: eyeDropperIndicator
            anchors.verticalCenter: parent.verticalCenter
            icon: StudioTheme.Constants.eyeDropper
            pixelSize: StudioTheme.Values.myIconFontSize * 1.4
            tooltip: qsTr("Eye Dropper")
            onClicked: ColorPaletteBackend.eyeDropper()
        }
    }

    function initEditor() {
        if (root.supportGradient && gradientModel.hasGradient) {
            colorEditor.color = gradientLine.currentColor
            gradientLine.currentColor = colorEditor.color
            hexTextField.text = colorEditor.color
            popupHexTextField.text = colorEditor.color
        }

        root.isInValidState = true
        colorEditor.originalColor = colorEditor.color
        colorPalette.selectedColor = colorEditor.color
        colorPicker.color = colorEditor.color

        root.createModel()
        root.determineActiveColorMode()
    }

    function commitGradientColor() {
        var hexColor = convertColorToString(colorEditor.color)
        root.popupHexTextField.text = hexColor
        root.gradientLine.currentColor = colorEditor.color
    }

    function isSolid() { return ceMode.currentValue === "Solid" }
    function isLinearGradient() { return ceMode.currentValue === "LinearGradient" }
    function isConicalGradient() { return ceMode.currentValue === "ConicalGradient" }
    function isRadialGradient() { return ceMode.currentValue === "RadialGradient" }

    function createModel() {
        // Build the color editor combobox model
        ceMode.items.clear()
        ceMode.items.append({
                                value: "Solid",
                                text: qsTr("Solid"),
                                enabled: true
                            })
        ceMode.items.append({
                                value: "LinearGradient",
                                text: qsTr("Linear"),
                                enabled: root.supportGradient
                            })
        ceMode.items.append({
                                value: "RadialGradient",
                                text: qsTr("Radial"),
                                enabled: root.supportGradient && root.shapeGradients
                            })
        ceMode.items.append({
                                value: "ConicalGradient",
                                text: qsTr("Conical"),
                                enabled: root.supportGradient && root.shapeGradients
                            })
    }

    function determineActiveColorMode() {
        if (root.supportGradient && gradientModel.hasGradient) {
            if (root.shapeGradients) {
                switch (gradientModel.gradientTypeName) {
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

    function updateThumbnail() {
        if (!gradientModel.hasGradient)
            return

        if (!root.shapeGradients) {
            var gradientString = "import QtQuick 2.15; Gradient {"
            var orientation = root.gradientOrientation.currentValue
                    === Gradient.Horizontal ? "Gradient.Horizontal" : "Gradient.Vertical"
            gradientString += "orientation: " + orientation + ";"

            for (var i = 0; i < gradientModel.count; i++)
                gradientString += "GradientStop {}"

            gradientString += "}"

            var gradientObject = Qt.createQmlObject(
                        gradientString, colorEditor.gradientThumbnail,
                        "dynamicGradient")

            for (i = 0; i < gradientModel.count; i++) {
                gradientObject.stops[i].color = gradientModel.getColor(
                            i)
                gradientObject.stops[i].position = gradientModel.getPosition(
                            i)
            }

            colorEditor.gradientThumbnail.gradient = gradientObject
        } else {
            var gradientStr = "import QtQuick 2.15; import QtQuick.Shapes 1.15; "
                    + gradientModel.gradientTypeName + " {"

            if (gradientModel.gradientTypeName === "LinearGradient") {
                gradientStr += "x1: 0" + ";x2: " + shape.width + ";y1: 0"
                        + ";y2: " + shape.height + ";"
            } else if (gradientModel.gradientTypeName === "RadialGradient") {
                gradientStr += "centerX: " + shape.width * 0.5 + ";centerY: "
                        + shape.height * 0.5 + ";focalX: " + shape.width * 0.5 + ";focalY: "
                        + shape.height * 0.5 + ";centerRadius: " + Math.min(
                            shape.width, shape.height) * 0.5 + ";focalRadius: 0" + ";"
            } else if (gradientModel.gradientTypeName === "ConicalGradient") {
                gradientStr += "centerX: " + shape.width * 0.5 + ";centerY: "
                        + shape.height * 0.5 + ";angle: 0" + ";"
            }

            for (var j = 0; j < gradientModel.count; j++)
                gradientStr += "GradientStop {}"

            gradientStr += "}"

            var gradientObj = Qt.createQmlObject(
                        gradientStr, colorEditor.shapeGradientThumbnail,
                        "dynamicShapeGradient")

            for (j = 0; j < gradientModel.count; j++) {
                gradientObj.stops[j].color = gradientModel.getColor(
                            j)
                gradientObj.stops[j].position = gradientModel.getPosition(
                            j)
            }

            colorEditor.shapeGradientThumbnail.fillGradient = gradientObj
        }
    }

    StudioControls.Menu {
        id: contextMenu

        StudioControls.MenuItem {
            text: qsTr("Open Color Dialog")
            onTriggered: colorPalette.showColorDialog(colorEditor.color)
        }
    }

    GradientModel {
        id: gradientModel
        anchorBackendProperty: anchorBackend
        gradientPropertyName: "gradient"
    }

    Column {
        id: colorColumn
        bottomPadding: StudioTheme.Values.popupMargin
        width: root.width
        spacing: StudioTheme.Values.colorEditorPopupSpacing

        GradientLine {
            id: gradientLine

            width: parent.width
            visible: !root.isSolid()

            model: gradientModel

            onCurrentColorChanged: {
                if (root.supportGradient && gradientModel.hasGradient) {
                    colorEditor.color = gradientLine.currentColor
                    colorPicker.color = colorEditor.color
                }
            }

            onHasGradientChanged: {
                if (!root.supportGradient)
                    return

                root.determineActiveColorMode()
            }

            onSelectedNodeChanged: {
                if (root.supportGradient && gradientModel.hasGradient)
                    colorEditor.originalColor = gradientLine.currentColor
            }

            onInvalidated: root.updateThumbnail()

            Connections {
                target: modelNodeBackend
                function onSelectionToBeChanged() {
                    colorEditorTimer.stop()
                    root.isInValidState = false

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

                function onSelectionChanged() {
                    root.initEditor()
                }
            }
        }

        StudioControls.ColorPicker {
            id: colorPicker

            width: parent.width
            sliderMargins: 4

            onUpdateColor: {
                colorEditor.color = colorPicker.color

                if (contextMenu.opened)
                    contextMenu.close()
            }
            onRightMouseButtonClicked: contextMenu.popup(colorPicker)

            onColorInvalidated: {
                hslHueSpinBox.value = colorPicker.hue
                hslSaturationSpinBox.value = colorPicker.saturationHSL
                hslLightnessSpinBox.value = colorPicker.lightness
                hslAlphaSpinBox.value = colorPicker.alpha

                redSpinBox.value = (colorPicker.red * 255)
                greenSpinBox.value = (colorPicker.green * 255)
                blueSpinBox.value = (colorPicker.blue * 255)
                rgbAlphaSpinBox.value = (colorPicker.alpha * 255)

                hsvHueSpinBox.value = colorPicker.hue
                hsvSaturationSpinBox.value = colorPicker.saturationHSV
                hsvValueSpinBox.value = colorPicker.value
                hsvAlphaSpinBox.value = colorPicker.alpha
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
                    width: root.twoColumnWidth
                }

                Label {
                    text: qsTr("New")
                    width: root.twoColumnWidth
                }
            }

            RowLayout {
                width: parent.width
                Layout.alignment: Qt.AlignTop
                spacing: StudioTheme.Values.controlGap

                Rectangle {
                    id: originalColorRectangle
                    color: colorEditor.originalColor
                    width: root.twoColumnWidth
                    height: StudioTheme.Values.height
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
                    width: root.twoColumnWidth
                    height: StudioTheme.Values.height
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
                            contextMenuFavorite.currentColor = colorEditor.color
                            contextMenuFavorite.popup()
                        }
                    }
                }
            }

            StudioControls.Menu {
                id: contextMenuFavorite

                property color currentColor

                StudioControls.MenuItem {
                    text: qsTr("Add to Favorites")
                    onTriggered: ColorPaletteBackend.addFavoriteColor(contextMenuFavorite.currentColor)
                }
            }
        }
    }

    Column {
        id: sectionColumn
        width: root.width

        StudioControls.Section {
            caption: qsTr("Color Details")
            anchors.left: parent.left
            anchors.right: parent.right

            Column {
                spacing: StudioTheme.Values.colorEditorPopupSpacing

                Row {
                    spacing: StudioTheme.Values.controlGap

                    StudioControls.ComboBox {
                        id: colorMode
                        implicitWidth: root.twoColumnWidth
                        width: colorMode.implicitWidth
                        actionIndicatorVisible: false
                        textRole: "text"
                        valueRole: "value"
                        model: [
                            { value: StudioControls.ColorPicker.Mode.HSVA, text: "HSVA" },
                            { value: StudioControls.ColorPicker.Mode.RGBA, text: "RGBA" },
                            { value: StudioControls.ColorPicker.Mode.HSLA, text: "HSLA" }
                        ]

                        onActivated: colorPicker.mode = colorMode.currentValue
                    }

                    LineEdit {
                        id: popupHexTextField
                        implicitWidth: root.twoColumnWidth
                        width: popupHexTextField.implicitWidth
                        writeValueManually: true
                        validator: RegularExpressionValidator {
                            regularExpression: /#[0-9A-Fa-f]{6}([0-9A-Fa-f]{2})?/g
                        }
                        showTranslateCheckBox: false
                        showExtendedFunctionButton: false
                        indicatorVisible: true
                        indicator.icon.text: StudioTheme.Constants.copy_small
                        indicator.onClicked: {
                            popupHexTextField.selectAll()
                            popupHexTextField.copy()
                            popupHexTextField.deselect()
                        }
                        backendValue: colorEditor.backendValue

                        onAccepted: colorEditor.color = colorFromString(popupHexTextField.text)
                        onCommitData: {
                            colorEditor.color = colorFromString(popupHexTextField.text)
                            if (root.isSolid()) {
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
                }

                Row {
                    id: rgbaRow
                    visible: colorPicker.mode === StudioControls.ColorPicker.Mode.RGBA
                    spacing: StudioTheme.Values.controlGap

                    DoubleSpinBox {
                        id: redSpinBox
                        width: root.fourColumnWidth
                        stepSize: 1
                        minimumValue: 0
                        maximumValue: 255
                        decimals: 0

                        onValueModified: {
                            var tmp = redSpinBox.value / 255.0
                            if (colorPicker.red !== tmp && !colorPicker.block) {
                                colorPicker.red = tmp
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        onDragStarted: colorEditorTimer.stop()
                        onIndicatorPressed: colorEditorTimer.stop()
                    }

                    DoubleSpinBox {
                        id: greenSpinBox
                        width: root.fourColumnWidth
                        stepSize: 1
                        minimumValue: 0
                        maximumValue: 255
                        decimals: 0

                        onValueModified: {
                            var tmp = greenSpinBox.value / 255.0
                            if (colorPicker.green !== tmp && !colorPicker.block) {
                                colorPicker.green = tmp
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        onDragStarted: colorEditorTimer.stop()
                        onIndicatorPressed: colorEditorTimer.stop()
                    }

                    DoubleSpinBox {
                        id: blueSpinBox
                        width: root.fourColumnWidth
                        stepSize: 1
                        minimumValue: 0
                        maximumValue: 255
                        decimals: 0

                        onValueModified: {
                            var tmp = blueSpinBox.value / 255.0
                            if (colorPicker.blue !== tmp && !colorPicker.block) {
                                colorPicker.blue = tmp
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        onDragStarted: colorEditorTimer.stop()
                        onIndicatorPressed: colorEditorTimer.stop()
                    }

                    DoubleSpinBox {
                        id: rgbAlphaSpinBox
                        width: root.fourColumnWidth
                        stepSize: 1
                        minimumValue: 0
                        maximumValue: 255
                        decimals: 0

                        onValueModified: {
                            var tmp = rgbAlphaSpinBox.value / 255.0
                            if (colorPicker.alpha !== tmp && !colorPicker.block) {
                                colorPicker.alpha = tmp
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        onDragStarted: colorEditorTimer.stop()
                        onIndicatorPressed: colorEditorTimer.stop()
                    }
                }

                Row {
                    id: hslaRow
                    visible: colorPicker.mode === StudioControls.ColorPicker.Mode.HSLA
                    spacing: StudioTheme.Values.controlGap

                    DoubleSpinBox {
                        id: hslHueSpinBox
                        width: root.fourColumnWidth
                        onValueModified: {
                            if (colorPicker.hue !== hslHueSpinBox.value
                                    && !colorPicker.block) {
                                colorPicker.hue = hslHueSpinBox.value
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        onDragStarted: colorEditorTimer.stop()
                        onIndicatorPressed: colorEditorTimer.stop()
                    }

                    DoubleSpinBox {
                        id: hslSaturationSpinBox
                        width: root.fourColumnWidth
                        onValueModified: {
                            if (colorPicker.saturationHSL !== hslSaturationSpinBox.value
                                    && !colorPicker.block) {
                                colorPicker.saturationHSL = hslSaturationSpinBox.value
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        onDragStarted: colorEditorTimer.stop()
                        onIndicatorPressed: colorEditorTimer.stop()
                    }

                    DoubleSpinBox {
                        id: hslLightnessSpinBox
                        width: root.fourColumnWidth
                        onValueModified: {
                            if (colorPicker.lightness !== hslLightnessSpinBox.value
                                    && !colorPicker.block) {
                                colorPicker.lightness = hslLightnessSpinBox.value
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        onDragStarted: colorEditorTimer.stop()
                        onIndicatorPressed: colorEditorTimer.stop()
                    }

                    DoubleSpinBox {
                        id: hslAlphaSpinBox
                        width: root.fourColumnWidth
                        onValueModified: {
                            if (colorPicker.alpha !== hslAlphaSpinBox.value
                                    && !colorPicker.block) {
                                colorPicker.alpha = hslAlphaSpinBox.value
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        onDragStarted: colorEditorTimer.stop()
                        onIndicatorPressed: colorEditorTimer.stop()
                    }
                }

                Row {
                    id: hsvaRow
                    visible: colorPicker.mode === StudioControls.ColorPicker.Mode.HSVA
                    spacing: StudioTheme.Values.controlGap

                    DoubleSpinBox {
                        id: hsvHueSpinBox
                        width: root.fourColumnWidth
                        onValueModified: {
                            if (colorPicker.hue !== hsvHueSpinBox.value
                                    && !colorPicker.block) {
                                colorPicker.hue = hsvHueSpinBox.value
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        onDragStarted: colorEditorTimer.stop()
                        onIndicatorPressed: colorEditorTimer.stop()
                    }

                    DoubleSpinBox {
                        id: hsvSaturationSpinBox
                        width: root.fourColumnWidth
                        onValueModified: {
                            if (colorPicker.saturationHSV !== hsvSaturationSpinBox.value
                                    && !colorPicker.block) {
                                colorPicker.saturationHSV = hsvSaturationSpinBox.value
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        onDragStarted: colorEditorTimer.stop()
                        onIndicatorPressed: colorEditorTimer.stop()
                    }

                    DoubleSpinBox {
                        id: hsvValueSpinBox
                        width: root.fourColumnWidth
                        onValueModified: {
                            if (colorPicker.value !== hsvValueSpinBox.value
                                    && !colorPicker.block) {
                                colorPicker.value = hsvValueSpinBox.value
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        onDragStarted: colorEditorTimer.stop()
                        onIndicatorPressed: colorEditorTimer.stop()
                    }

                    DoubleSpinBox {
                        id: hsvAlphaSpinBox
                        width: root.fourColumnWidth
                        onValueModified: {
                            if (colorPicker.alpha !== hsvAlphaSpinBox.value
                                    && !colorPicker.block) {
                                colorPicker.alpha = hsvAlphaSpinBox.value
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        onDragStarted: colorEditorTimer.stop()
                        onIndicatorPressed: colorEditorTimer.stop()
                    }
                }
            }
        }

        StudioControls.Section {
            id: sectionPalette
            caption: qsTr("Palette")
            anchors.left: parent.left
            anchors.right: parent.right
            bottomPadding: root.isSolid() ? 0 : sectionPalette.style.sectionHeadSpacerHeight
            collapsedBottomPadding: root.isSolid() ? 0 : StudioTheme.Values.border

            StudioControls.ColorPalette {
                id: colorPalette
                width: root.width
                twoColumnWidth: root.twoColumnWidth
                fourColumnWidth: root.fourColumnWidth

                enableSingletonConnection: root.visible
                onSelectedColorChanged: {
                    colorPicker.color = colorPalette.selectedColor
                    colorEditor.color = colorPalette.selectedColor
                }
                onDialogColorChanged: {
                    colorPicker.color = colorPalette.selectedColor
                    colorEditor.color = colorPalette.selectedColor
                }
            }
        }

        StudioControls.Section {
            id: gradientControls
            caption: qsTr("Gradient Controls")
            visible: !root.isSolid()
            anchors.left: parent.left
            anchors.right: parent.right
            bottomPadding: 0
            collapsedBottomPadding: 0

            component ControlsRow: Row {
                property alias propertyName: spinBox.propertyName
                property alias gradientTypeName: spinBox.gradientTypeName
                property alias labelText: label.text
                property alias labelTooltip: label.tooltip
                property alias value: spinBox.value

                spacing: StudioTheme.Values.controlGap

                Connections {
                    target: ceMode
                    function onActivated() {
                        spinBox.readValue()
                    }
                }

                Connections {
                    target: modelNodeBackend
                    function onSelectionChanged() {
                        spinBox.readValue()
                    }
                }

                ControlLabel {
                    id: label
                    anchors.verticalCenter: parent.verticalCenter
                    horizontalAlignment: Text.AlignRight
                    width: root.fourColumnWidth
                }

                GradientPropertySpinBox {
                    id: spinBox
                    spinBoxWidth: root.twoColumnWidth
                    unitWidth: root.fourColumnWidth
                }
            }

            // Default Gradient Controls
            Column {
                id: defaultGradientControls
                spacing: StudioTheme.Values.colorEditorPopupSpacing
                visible: root.isLinearGradient() && !colorEditor.shapeGradients

                Row {
                    id: defaultGradientOrientation
                    spacing: StudioTheme.Values.controlGap

                    StudioControls.ComboBox {
                        id: gradientOrientation
                        implicitWidth: root.twoColumnWidth
                        width: gradientOrientation.implicitWidth
                        model: [{ value: Gradient.Vertical, text: qsTr("Vertical") },
                            { value: Gradient.Horizontal, text: qsTr("Horizontal") }]
                        textRole: "text"
                        valueRole: "value"
                        actionIndicatorVisible: false

                        onActivated: {
                            gradientLine.model.setGradientOrientation(gradientOrientation.currentValue)
                            root.updateThumbnail()
                        }

                        Component.onCompleted: {
                            var orientation = gradientLine.model.readGradientOrientation()

                            if (orientation === "Horizontal") {
                                gradientOrientation.currentIndex =
                                        gradientOrientation.indexOfValue(Gradient.Horizontal)
                            } else {
                                gradientOrientation.currentIndex =
                                        gradientOrientation.indexOfValue(Gradient.Vertical)
                            }
                        }
                    }

                    IconLabel {
                        id: iconLabel
                        anchors.verticalCenter: parent.verticalCenter
                        icon: StudioTheme.Constants.orientation
                        pixelSize: StudioTheme.Values.myIconFontSize * 1.4
                        tooltip: qsTr("Defines the direction of the gradient.")
                    }
                }
            }

            // Linear Gradient Controls
            Column {
                id: linearGradientControls
                spacing: StudioTheme.Values.colorEditorPopupSpacing
                visible: root.isLinearGradient() && colorEditor.shapeGradients
                readonly property string gradientTypeName: "LinearGradient"

                ControlsRow {
                    propertyName: "x1"
                    gradientTypeName: linearGradientControls.gradientTypeName
                    labelText: "X1"
                    labelTooltip: qsTr("Defines the start point for color interpolation.")
                }

                ControlsRow {
                    propertyName: "x2"
                    gradientTypeName: linearGradientControls.gradientTypeName
                    labelText: "X2"
                    labelTooltip: qsTr("Defines the end point for color interpolation.")
                }

                ControlsRow {
                    propertyName: "y1"
                    gradientTypeName: linearGradientControls.gradientTypeName
                    labelText: "Y1"
                    labelTooltip: qsTr("Defines the start point for color interpolation.")
                }

                ControlsRow {
                    propertyName: "y2"
                    gradientTypeName: linearGradientControls.gradientTypeName
                    labelText: "Y2"
                    labelTooltip: qsTr("Defines the end point for color interpolation.")
                }
            }

            // Radial Gradient Controls
            Column {
                id: radialGradientControls
                spacing: StudioTheme.Values.colorEditorPopupSpacing
                visible: root.isRadialGradient()
                readonly property string gradientTypeName: "RadialGradient"

                ControlsRow {
                    propertyName: "centerX"
                    gradientTypeName: radialGradientControls.gradientTypeName
                    labelText: "CenterX"
                    labelTooltip: qsTr("Defines the center point.")
                }

                ControlsRow {
                    propertyName: "centerY"
                    gradientTypeName: radialGradientControls.gradientTypeName
                    labelText: "CenterY"
                    labelTooltip: qsTr("Defines the center point.")
                }

                ControlsRow {
                    propertyName: "focalX"
                    gradientTypeName: radialGradientControls.gradientTypeName
                    labelText: "FocalX"
                    labelTooltip: qsTr("Defines the focal point.")
                }

                ControlsRow {
                    propertyName: "focalY"
                    gradientTypeName: radialGradientControls.gradientTypeName
                    labelText: "FocalY"
                    labelTooltip: qsTr("Defines the focal point.")
                }

                ControlsRow {
                    propertyName: "centerRadius"
                    gradientTypeName: radialGradientControls.gradientTypeName
                    labelText: "Center Radius"
                    labelTooltip: qsTr("Defines the center radius.")
                }

                ControlsRow {
                    propertyName: "focalRadius"
                    gradientTypeName: radialGradientControls.gradientTypeName
                    labelText: "Focal Radius"
                    labelTooltip: qsTr("Defines the focal radius. Set to 0 for simple radial gradients.")
                }
            }

            // Conical Gradient Controls
            Column {
                id: conicalGradientControls
                spacing: StudioTheme.Values.colorEditorPopupSpacing
                visible: root.isConicalGradient()
                readonly property string gradientTypeName: "ConicalGradient"

                ControlsRow {
                    propertyName: "centerX"
                    gradientTypeName: conicalGradientControls.gradientTypeName
                    labelText: "CenterX"
                    labelTooltip: qsTr("Defines the center point.")
                }

                ControlsRow {
                    propertyName: "centerY"
                    gradientTypeName: conicalGradientControls.gradientTypeName
                    labelText: "CenterY"
                    labelTooltip: qsTr("Defines the center point.")
                }

                ControlsRow {
                    propertyName: "angle"
                    gradientTypeName: conicalGradientControls.gradientTypeName
                    labelText: "Angle"
                    labelTooltip: qsTr("Defines the start angle for the conical gradient. The value is in degrees (0-360).")
                }
            }
        }
    }
}
