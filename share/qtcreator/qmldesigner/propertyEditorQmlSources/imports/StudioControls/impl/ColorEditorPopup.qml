// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import QtQuickDesignerColorPalette

Column {
    id: root

    property color color
    property color originalColor

    readonly property real twoColumnWidth: (colorColumn.width - StudioTheme.Values.controlGap) * 0.5
    readonly property real fourColumnWidth: (colorColumn.width - (3 * StudioTheme.Values.controlGap)) * 0.25

    property Item titleBarContent: Row {
        anchors.fill: parent
        spacing: 10

        StudioControls.IconIndicator {
            id: transparentIndicator
            anchors.verticalCenter: parent.verticalCenter
            icon: StudioTheme.Constants.transparent
            pixelSize: StudioTheme.Values.myIconFontSize * 1.4
            toolTip: qsTr("Transparent")

            onClicked: {
                colorPicker.alpha = 0
                colorPicker.invalidateColor()
                colorPicker.updateColor()
            }
        }

        StudioControls.IconIndicator {
            id: eyeDropperIndicator
            anchors.verticalCenter: parent.verticalCenter
            icon: StudioTheme.Constants.eyeDropper
            pixelSize: StudioTheme.Values.myIconFontSize * 1.4
            toolTip: qsTr("Eye Dropper")
            onClicked: ColorPaletteBackend.eyeDropper()
        }
    }

    onColorChanged: {
        colorPicker.color = root.color
        hexTextField.text = root.color
    }

    signal activateColor(var color)

    StudioControls.Menu {
        id: contextMenu

        StudioControls.MenuItem {
            text: qsTr("Open Color Dialog")
            onTriggered: colorPalette.showColorDialog(colorEditor.color)
        }
    }

    Column {
        id: colorColumn
        bottomPadding: StudioTheme.Values.popupMargin
        width: root.width
        spacing: StudioTheme.Values.colorEditorPopupSpacing

        ColorPicker {
            id: colorPicker

            width: parent.width
            sliderMargins: 4

            onUpdateColor: {
                root.activateColor(colorPicker.color)

                if (contextMenu.opened)
                    contextMenu.close()
            }
            onRightMouseButtonClicked: contextMenu.popup(colorPicker)

            onColorInvalidated: {
                hslHueSpinBox.realValue = colorPicker.hue
                hslSaturationSpinBox.realValue = colorPicker.saturationHSL
                hslLightnessSpinBox.realValue = colorPicker.lightness
                hslAlphaSpinBox.realValue = colorPicker.alpha

                redSpinBox.realValue = (colorPicker.red * 255)
                greenSpinBox.realValue = (colorPicker.green * 255)
                blueSpinBox.realValue = (colorPicker.blue * 255)
                rgbAlphaSpinBox.realValue = (colorPicker.alpha * 255)

                hsvHueSpinBox.realValue = colorPicker.hue
                hsvSaturationSpinBox.realValue = colorPicker.saturationHSV
                hsvValueSpinBox.realValue = colorPicker.value
                hsvAlphaSpinBox.realValue = colorPicker.alpha
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
                    color: StudioTheme.Values.themeTextColor
                    elide: Text.ElideRight
                    font.pixelSize: StudioTheme.Values.myFontSize
                    Layout.preferredWidth: width
                    Layout.minimumWidth: width
                    Layout.maximumWidth: width
                }

                Label {
                    text: qsTr("New")
                    width: root.twoColumnWidth
                    color: StudioTheme.Values.themeTextColor
                    elide: Text.ElideRight
                    font.pixelSize: StudioTheme.Values.myFontSize
                    Layout.preferredWidth: width
                    Layout.minimumWidth: width
                    Layout.maximumWidth: width
                }
            }

            RowLayout {
                width: parent.width
                Layout.alignment: Qt.AlignTop
                spacing: StudioTheme.Values.controlGap

                Rectangle {
                    id: originalColorRectangle
                    color: root.originalColor
                    width: root.twoColumnWidth
                    height: StudioTheme.Values.height
                    border.width: StudioTheme.Values.border
                    border.color: StudioTheme.Values.themeControlOutline

                    Image {
                        anchors.fill: parent
                        source: "qrc:/navigator/icon/checkers.png"
                        fillMode: Image.Tile
                        z: -1
                    }

                    StudioControls.ToolTipArea {
                        anchors.fill: parent
                        text: originalColorRectangle.color
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: function(mouse) {
                            if (mouse.button === Qt.LeftButton)
                                root.activateColor(root.originalColor)

                            if (mouse.button === Qt.RightButton) {
                                contextMenuFavorite.currentColor = root.originalColor
                                contextMenuFavorite.popup()
                            }
                        }
                    }
                }

                Rectangle {
                    id: newColorRectangle
                    color: root.color
                    width: root.twoColumnWidth
                    height: StudioTheme.Values.height
                    border.width: StudioTheme.Values.border
                    border.color: StudioTheme.Values.themeControlOutline

                    Image {
                        anchors.fill: parent
                        source: "qrc:/navigator/icon/checkers.png"
                        fillMode: Image.Tile
                        z: -1
                    }

                    StudioControls.ToolTipArea {
                        anchors.fill: parent
                        text: newColorRectangle.color
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
                            { value: ColorPicker.Mode.HSVA, text: "HSVA" },
                            { value: ColorPicker.Mode.RGBA, text: "RGBA" },
                            { value: ColorPicker.Mode.HSLA, text: "HSLA" }
                        ]

                        onActivated: colorPicker.mode = colorMode.currentValue
                    }

                    StudioControls.TextField {
                        id: hexTextField
                        implicitWidth: root.twoColumnWidth
                        width: hexTextField.implicitWidth
                        actionIndicatorVisible: false
                        translationIndicatorVisible: false
                        indicatorVisible: true
                        indicator.icon.text: StudioTheme.Constants.copy_small
                        indicator.onClicked: {
                            hexTextField.selectAll()
                            hexTextField.copy()
                            hexTextField.deselect()
                        }

                        validator: RegularExpressionValidator {
                            regularExpression: /#[0-9A-Fa-f]{6}([0-9A-Fa-f]{2})?/g
                        }

                        onEditingFinished: root.activateColor(colorFromString(hexTextField.text))
                    }
                }

                Row {
                    id: rgbaRow
                    visible: colorPicker.mode === ColorPicker.Mode.RGBA
                    spacing: StudioTheme.Values.controlGap

                    StudioControls.RealSpinBox {
                        id: redSpinBox
                        width: root.fourColumnWidth
                        realStepSize: 1
                        realFrom: 0
                        realTo: 255
                        decimals: 0
                        actionIndicatorVisible: false

                        onRealValueModified: {
                            var tmp = redSpinBox.realValue / 255.0
                            if (colorPicker.red !== tmp && !colorPicker.block) {
                                colorPicker.red = tmp
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        //onDragStarted: colorEditorTimer.stop()
                        //onIndicatorPressed: colorEditorTimer.stop()
                    }

                    StudioControls.RealSpinBox {
                        id: greenSpinBox
                        width: root.fourColumnWidth
                        realStepSize: 1
                        realFrom: 0
                        realTo: 255
                        decimals: 0
                        actionIndicatorVisible: false

                        onRealValueModified: {
                            var tmp = greenSpinBox.realValue / 255.0
                            if (colorPicker.green !== tmp && !colorPicker.block) {
                                colorPicker.green = tmp
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        //onDragStarted: colorEditorTimer.stop()
                        //onIndicatorPressed: colorEditorTimer.stop()
                    }

                    StudioControls.RealSpinBox {
                        id: blueSpinBox
                        width: root.fourColumnWidth
                        realStepSize: 1
                        realFrom: 0
                        realTo: 255
                        decimals: 0
                        actionIndicatorVisible: false

                        onRealValueModified: {
                            var tmp = blueSpinBox.realValue / 255.0
                            if (colorPicker.blue !== tmp && !colorPicker.block) {
                                colorPicker.blue = tmp
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        //onDragStarted: colorEditorTimer.stop()
                        //onIndicatorPressed: colorEditorTimer.stop()
                    }

                    StudioControls.RealSpinBox {
                        id: rgbAlphaSpinBox
                        width: root.fourColumnWidth
                        realStepSize: 1
                        realFrom: 0
                        realTo: 255
                        decimals: 0
                        actionIndicatorVisible: false

                        onRealValueModified: {
                            var tmp = rgbAlphaSpinBox.realValue / 255.0
                            if (colorPicker.alpha !== tmp && !colorPicker.block) {
                                colorPicker.alpha = tmp
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        //onDragStarted: colorEditorTimer.stop()
                        //onIndicatorPressed: colorEditorTimer.stop()
                    }
                }

                Row {
                    id: hslaRow
                    visible: colorPicker.mode === ColorPicker.Mode.HSLA
                    spacing: StudioTheme.Values.controlGap

                    StudioControls.RealSpinBox {
                        id: hslHueSpinBox
                        width: root.fourColumnWidth
                        realFrom: 0.0
                        realTo: 1.0
                        realStepSize: 0.1
                        decimals: 2
                        actionIndicatorVisible: false

                        onRealValueModified: {
                            if (colorPicker.hue !== hslHueSpinBox.realValue
                                    && !colorPicker.block) {
                                colorPicker.hue = hslHueSpinBox.realValue
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        //onDragStarted: colorEditorTimer.stop()
                        //onIndicatorPressed: colorEditorTimer.stop()
                    }

                    StudioControls.RealSpinBox {
                        id: hslSaturationSpinBox
                        width: root.fourColumnWidth
                        realFrom: 0.0
                        realTo: 1.0
                        realStepSize: 0.1
                        decimals: 2
                        actionIndicatorVisible: false

                        onRealValueModified: {
                            if (colorPicker.saturationHSL !== hslSaturationSpinBox.realValue
                                    && !colorPicker.block) {
                                colorPicker.saturationHSL = hslSaturationSpinBox.realValue
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        //onDragStarted: colorEditorTimer.stop()
                        //onIndicatorPressed: colorEditorTimer.stop()
                    }

                    StudioControls.RealSpinBox {
                        id: hslLightnessSpinBox
                        width: root.fourColumnWidth
                        realFrom: 0.0
                        realTo: 1.0
                        realStepSize: 0.1
                        decimals: 2
                        actionIndicatorVisible: false

                        onValueModified: {
                            if (colorPicker.lightness !== hslLightnessSpinBox.realValue
                                    && !colorPicker.block) {
                                colorPicker.lightness = hslLightnessSpinBox.realValue
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        //onDragStarted: colorEditorTimer.stop()
                        //onIndicatorPressed: colorEditorTimer.stop()
                    }

                    StudioControls.RealSpinBox {
                        id: hslAlphaSpinBox
                        width: root.fourColumnWidth
                        realFrom: 0.0
                        realTo: 1.0
                        realStepSize: 0.1
                        decimals: 2
                        actionIndicatorVisible: false

                        onRealValueModified: {
                            if (colorPicker.alpha !== hslAlphaSpinBox.realValue
                                    && !colorPicker.block) {
                                colorPicker.alpha = hslAlphaSpinBox.realValue
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        //onDragStarted: colorEditorTimer.stop()
                        //onIndicatorPressed: colorEditorTimer.stop()
                    }
                }

                Row {
                    id: hsvaRow
                    visible: colorPicker.mode === ColorPicker.Mode.HSVA
                    spacing: StudioTheme.Values.controlGap

                    StudioControls.RealSpinBox {
                        id: hsvHueSpinBox
                        width: root.fourColumnWidth
                        realFrom: 0.0
                        realTo: 1.0
                        realStepSize: 0.1
                        decimals: 2
                        actionIndicatorVisible: false

                        onRealValueModified: {
                            if (colorPicker.hue !== hsvHueSpinBox.realValue
                                    && !colorPicker.block) {
                                colorPicker.hue = hsvHueSpinBox.realValue
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        //onDragStarted: colorEditorTimer.stop()
                        //onIndicatorPressed: colorEditorTimer.stop()
                    }

                    StudioControls.RealSpinBox {
                        id: hsvSaturationSpinBox
                        width: root.fourColumnWidth
                        realFrom: 0.0
                        realTo: 1.0
                        realStepSize: 0.1
                        decimals: 2
                        actionIndicatorVisible: false

                        onRealValueModified: {
                            if (colorPicker.saturationHSV !== hsvSaturationSpinBox.realValue
                                    && !colorPicker.block) {
                                colorPicker.saturationHSV = hsvSaturationSpinBox.realValue
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        //onDragStarted: colorEditorTimer.stop()
                        //onIndicatorPressed: colorEditorTimer.stop()
                    }

                    StudioControls.RealSpinBox {
                        id: hsvValueSpinBox
                        width: root.fourColumnWidth
                        realFrom: 0.0
                        realTo: 1.0
                        realStepSize: 0.1
                        decimals: 2
                        actionIndicatorVisible: false

                        onRealValueModified: {
                            if (colorPicker.value !== hsvValueSpinBox.realValue
                                    && !colorPicker.block) {
                                colorPicker.value = hsvValueSpinBox.realValue
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        //onDragStarted: colorEditorTimer.stop()
                        //onIndicatorPressed: colorEditorTimer.stop()
                    }

                    StudioControls.RealSpinBox {
                        id: hsvAlphaSpinBox
                        width: root.fourColumnWidth
                        realFrom: 0.0
                        realTo: 1.0
                        realStepSize: 0.1
                        decimals: 2
                        actionIndicatorVisible: false

                        onRealValueModified: {
                            if (colorPicker.alpha !== hsvAlphaSpinBox.realValue
                                    && !colorPicker.block) {
                                colorPicker.alpha = hsvAlphaSpinBox.realValue
                                colorPicker.invalidateColor()
                                colorPicker.updateColor()
                            }
                        }
                        //onDragStarted: colorEditorTimer.stop()
                        //onIndicatorPressed: colorEditorTimer.stop()
                    }
                }
            }
        }

        StudioControls.Section {
            caption: qsTr("Palette")
            anchors.left: parent.left
            anchors.right: parent.right
            bottomPadding: 0
            collapsedBottomPadding: 0

            StudioControls.ColorPalette {
                id: colorPalette

                width: root.width

                twoColumnWidth: root.twoColumnWidth
                fourColumnWidth: root.fourColumnWidth

                enableSingletonConnection: root.visible
                onSelectedColorChanged: root.activateColor(colorPalette.selectedColor)
                onDialogColorChanged: root.activateColor(colorPalette.selectedColor)
            }
        }
    }
}
