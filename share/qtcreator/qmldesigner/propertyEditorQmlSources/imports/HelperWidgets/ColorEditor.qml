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
        if (colorEditor.backendValue === undefined || colorEditor.backendValue.value === undefined)
            return "white" // default color for Rectangle

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
            enabled: true
        })
        ceMode.items.append({
            value: "LinearGradient",
            text: qsTr("Linear"),
            enabled: colorEditor.supportGradient
        })
        ceMode.items.append({
            value: "RadialGradient",
            text: qsTr("Radial"),
            enabled: colorEditor.supportGradient && colorEditor.shapeGradients
        })
        ceMode.items.append({
            value: "ConicalGradient",
            text: qsTr("Conical"),
            enabled: colorEditor.supportGradient && colorEditor.shapeGradients
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

    Connections {
        id: backendConnection
        target: colorEditor

        function onValueChanged() {
            if (isNotInGradientMode())
                colorEditor.color = colorEditor.value
        }

        function onBackendValueChanged() {
            if (isNotInGradientMode())
                colorEditor.color = colorEditor.value
        }
    }

    Timer {
        id: colorEditorTimer
        repeat: false
        interval: 100
        running: false
        onTriggered: {
            backendConnection.enabled = false

            if (colorEditor.backendValue !== undefined) {
                if (colorEditor.isVector3D)
                    colorEditor.backendValue.value = Qt.vector3d(colorEditor.color.r,
                                                                 colorEditor.color.g,
                                                                 colorEditor.color.b)
                else
                    colorEditor.backendValue.value = colorEditor.color
            }

            backendConnection.enabled = true
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

            // This connection is meant to update the popups y-position and the main scrollviews
            // height as soon as the height of the color picker changes. Initially the height of the
            // color picker is 0 until its completion is done.
            Connections {
                target: colorPicker
                function onHeightChanged() {
                    cePopup.setPopupY()
                    cePopup.setMainScrollViewHeight()
                }
            }

            onOpened: {
                cePopup.setPopupY()
                cePopup.setMainScrollViewHeight()
            }
            onYChanged: cePopup.setMainScrollViewHeight()
            onHeightChanged: cePopup.setMainScrollViewHeight()

            function setMainScrollViewHeight() {
                if (Controller.mainScrollView === null)
                    return

                var mapped = preview.mapToItem(Controller.mainScrollView.contentItem, cePopup.x, cePopup.y)
                Controller.mainScrollView.temporaryHeight = mapped.y + cePopup.height
                        + StudioTheme.Values.colorEditorPopupMargin
            }

            function setPopupY() {
                if (Controller.mainScrollView === null)
                    return

                var tmp = preview.mapToItem(Controller.mainScrollView.contentItem, preview.x, preview.y)
                cePopup.y = Math.max(-tmp.y + StudioTheme.Values.colorEditorPopupMargin,
                                     cePopup.__defaultY)
            }

            onClosed: Controller.mainScrollView.temporaryHeight = 0

            property real __defaultX: - StudioTheme.Values.colorEditorPopupWidth * 0.5
                                      + preview.width * 0.5
            property real __defaultY: - StudioTheme.Values.colorEditorPopupPadding
                                      - (StudioTheme.Values.colorEditorPopupSpacing * 2)
                                      - StudioTheme.Values.defaultControlHeight
                                      - StudioTheme.Values.colorEditorPopupLineHeight
                                      - colorPicker.height * 0.5
                                      + preview.height * 0.5

            x: cePopup.__defaultX
            y: cePopup.__defaultY

            width: StudioTheme.Values.colorEditorPopupWidth
            height: colorColumn.height + sectionColumn.height
                    + StudioTheme.Values.colorEditorPopupPadding + 2 // TODO magic number

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
                    anchors.margins: StudioTheme.Values.colorEditorPopupPadding
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
                            tooltip: qsTr("Transparent")
                            onClicked: {
                                colorPicker.alpha = 0
                                colorPicker.invalidateColor()
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
                            onClicked: ColorPaletteBackend.eyeDropper()
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
                            if (colorEditor.supportGradient && gradientLine.hasGradient) {
                                colorEditor.color = gradientLine.currentColor
                                colorPicker.color = colorEditor.color
                            }
                        }

                        onHasGradientChanged: {
                            if (!colorEditor.supportGradient)
                                return

                            colorEditor.determineActiveColorMode()
                        }

                        onSelectedNodeChanged: {
                            if (colorEditor.supportGradient && gradientLine.hasGradient)
                                colorEditor.originalColor = gradientLine.currentColor
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
                                    gradientLine.currentColor = colorEditor.color
                                    hexTextField.text = colorEditor.color
                                    popupHexTextField.text = colorEditor.color
                                }

                                gradientLine.isInValidState = true
                                colorEditor.originalColor = colorEditor.color
                                colorPalette.selectedColor = colorEditor.color
                                colorPicker.color = colorEditor.color

                                colorEditor.createModel()
                                colorEditor.determineActiveColorMode()
                            }
                        }
                    }

                    ColorPicker {
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
                                onTriggered: ColorPaletteBackend.addFavoriteColor(
                                                 contextMenuFavorite.currentColor)
                            }
                        }
                    }
                }

                Column {
                    id: sectionColumn
                    anchors.topMargin: StudioTheme.Values.colorEditorPopupPadding
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
                                    width: 2 * StudioTheme.Values.colorEditorPopupSpinBoxWidth
                                           + StudioTheme.Values.controlGap
                                    horizontalAlignment: Text.AlignLeft
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
                                    textRole: "text"
                                    valueRole: "value"
                                    model: [
                                        { value: ColorPicker.Mode.HSVA, text: "HSVA" },
                                        { value: ColorPicker.Mode.RGBA, text: "RGBA" },
                                        { value: ColorPicker.Mode.HSLA, text: "HSLA" }
                                    ]

                                    onActivated: colorPicker.mode = colorMode.currentValue
                                }
                            }

                            RowLayout {
                                id: rgbaRow
                                visible: colorPicker.mode === ColorPicker.Mode.RGBA
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
                                    width: StudioTheme.Values.colorEditorPopupSpinBoxWidth
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
                                    width: StudioTheme.Values.colorEditorPopupSpinBoxWidth
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
                                    width: StudioTheme.Values.colorEditorPopupSpinBoxWidth
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

                            RowLayout {
                                id: hslaRow
                                visible: colorPicker.mode === ColorPicker.Mode.HSLA
                                Layout.fillWidth: true
                                spacing: StudioTheme.Values.controlGap

                                DoubleSpinBox {
                                    id: hslHueSpinBox
                                    width: StudioTheme.Values.colorEditorPopupSpinBoxWidth
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
                                    width: StudioTheme.Values.colorEditorPopupSpinBoxWidth
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
                                    width: StudioTheme.Values.colorEditorPopupSpinBoxWidth
                                    onValueModified: {
                                        if (colorPicker.lightness !== hslLightnessSpinBox.value
                                                && !colorPicker.block) {
                                            colorPicker.lightness = hslLightnessSpinBox.value
                                            colorPicker.invalidateColor()
                                            colorPicker.updateColor()
                                        }
                                    }
                                }

                                DoubleSpinBox {
                                    id: hslAlphaSpinBox
                                    width: StudioTheme.Values.colorEditorPopupSpinBoxWidth
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

                            RowLayout {
                                id: hsvaRow
                                visible: colorPicker.mode === ColorPicker.Mode.HSVA
                                Layout.fillWidth: true
                                spacing: StudioTheme.Values.controlGap

                                DoubleSpinBox {
                                    id: hsvHueSpinBox
                                    width: StudioTheme.Values.colorEditorPopupSpinBoxWidth
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
                                    width: StudioTheme.Values.colorEditorPopupSpinBoxWidth
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
                                    width: StudioTheme.Values.colorEditorPopupSpinBoxWidth
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
                                    width: StudioTheme.Values.colorEditorPopupSpinBoxWidth
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
