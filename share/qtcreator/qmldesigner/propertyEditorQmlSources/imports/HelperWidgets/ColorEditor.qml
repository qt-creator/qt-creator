// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuick.Shapes
import QtQuick.Templates as T
import StudioTheme as StudioTheme
import StudioControls as StudioControls
import QtQuickDesignerColorPalette

SecondColumnLayout {
    id: colorEditor

    property color color
    property bool supportGradient: false
    property variant backendValue

    property variant value: {
        if (!colorEditor.backendValue || !colorEditor.backendValue.value)
            return "white" // default color for Rectangle

        if (colorEditor.isVector3D) {
            return Qt.rgba(colorEditor.backendValue.value.x,
                           colorEditor.backendValue.value.y,
                           colorEditor.backendValue.value.z, 1)
        }

        return colorEditor.backendValue.value
    }

    property alias gradientPropertyName: popupDialog.gradientPropertyName

    property alias gradientThumbnail: gradientThumbnail
    property alias shapeGradientThumbnail: shapeGradientThumbnail

    property alias showExtendedFunctionButton: hexTextField.showExtendedFunctionButton
    property alias showHexTextField: hexTextField.visible

    property bool shapeGradients: false
    property color originalColor
    property bool isVector3D: false

    property alias spacer: spacer

    property bool __block: false

    property string caption // Legacy Qt5 specifics sheets compatibility

    function resetShapeColor() {
        colorEditor.backendValue.resetValue()
    }

    function initEditor() {
        colorEditor.syncColor()
    }

    // Syncing color from backend to frontend and block reflection
    function syncColor() {
        colorEditor.__block = true
        colorEditor.color = colorEditor.value
        colorEditor.__block = false
    }

    Connections {
        id: backendConnection
        target: colorEditor

        function onValueChanged() {
            if (popupDialog.isSolid())
                colorEditor.syncColor()
        }

        function onBackendValueChanged() {
            if (popupDialog.isSolid())
                colorEditor.syncColor()
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
                    colorEditor.backendValue.value = Qt.vector3d(
                                colorEditor.color.r, colorEditor.color.g,
                                colorEditor.color.b)
                else
                    colorEditor.backendValue.value = colorEditor.color
            }

            backendConnection.enabled = true
        }
    }

    onColorChanged: {
        if (colorEditor.__block)
            return

        if (!popupDialog.isInValidState)
            return

        popupDialog.commitToGradient()

        // Delay setting the color to keep ui responsive
        if (popupDialog.isSolid())
            colorEditorTimer.restart()
    }

    Spacer {
        implicitWidth: StudioTheme.Values.actionIndicatorWidth
    }

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
            visible: !popupDialog.isSolid()
                     && !colorEditor.shapeGradients
                     && popupDialog.isLinearGradient()
        }

        Shape {
            id: shape
            anchors.fill: parent
            anchors.margins: StudioTheme.Values.border
            visible: !popupDialog.isSolid() && colorEditor.shapeGradients

            ShapePath {
                id: shapeGradientThumbnail
                startX: shape.x - 1
                startY: shape.y - 1
                strokeWidth: -1
                strokeColor: "green"

                PathLine {
                    x: shape.x - 1
                    y: shape.height
                }
                PathLine {
                    x: shape.width
                    y: shape.height
                }
                PathLine {
                    x: shape.width
                    y: shape.y - 1
                }
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
                popupDialog.visibility ? popupDialog.close() : popupDialog.open()
                forceActiveFocus()
            }
        }

        StudioControls.PopupDialog {
            id: popupDialog

            property bool isInValidState: loader.active ? popupDialog.loaderItem.isInValidState : true
            property QtObject loaderItem: loader.item
            property string gradientPropertyName

            keepOpen: loader.item?.eyeDropperActive ?? false

            width: 260

            function commitToGradient() {
                if (!loader.active)
                    return

                if (colorEditor.supportGradient && popupDialog.loaderItem.gradientModel.hasGradient) {
                    var hexColor = convertColorToString(colorEditor.color)
                    hexTextField.text = hexColor
                    popupDialog.loaderItem.commitGradientColor()
                }
            }

            function isSolid() {
                 if (!loader.active)
                     return true

                 return popupDialog.loaderItem.isSolid()
            }

            function isLinearGradient(){
                if (!loader.active)
                    return false

                return popupDialog.loaderItem.isLinearGradient()
            }

            function ensureLoader() {
                if (!loader.active)
                    loader.active = true
            }

            function open() {
                popupDialog.ensureLoader()
                popupDialog.show(preview)
            }

            function determineActiveColorMode() {
                if (loader.active && popupDialog.loaderItem)
                    popupDialog.loaderItem.determineActiveColorMode()
                else
                    colorEditor.syncColor()
            }

            Loader {
                id: loader
                active: colorEditor.supportGradient

                sourceComponent: ColorEditorPopup {
                    shapeGradients: colorEditor.shapeGradients
                    supportGradient: colorEditor.supportGradient
                    width: popupDialog.contentWidth
                    visible: popupDialog.visible
                }

                onLoaded: {
                    popupDialog.loaderItem.initEditor()
                    popupDialog.titleBar = loader.item.titleBarContent
                }
            }
        }
    }

    Spacer {
        implicitWidth: StudioTheme.Values.twoControlColumnGap
    }

    LineEdit {
        id: hexTextField
        implicitWidth: StudioTheme.Values.twoControlColumnWidth
                       + StudioTheme.Values.actionIndicatorWidth
        width: hexTextField.implicitWidth
        enabled: popupDialog.isSolid()
        writeValueManually: true
        validator: RegularExpressionValidator {
            regularExpression: /#[0-9A-Fa-f]{6}([0-9A-Fa-f]{2})?/g
        }
        showTranslateCheckBox: false
        indicatorVisible: true
        indicator.icon.text: StudioTheme.Constants.copy_small
        indicator.onClicked: {
            hexTextField.selectAll()
            hexTextField.copy()
            hexTextField.deselect()
        }
        backendValue: colorEditor.backendValue

        onAccepted: colorEditor.color = colorFromString(hexTextField.text)
        onCommitData: {
            colorEditor.color = colorFromString(hexTextField.text)
            if (popupDialog.isSolid()) {
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

    ExpandingSpacer {
        id: spacer
    }

    Component.onCompleted: popupDialog.determineActiveColorMode()

    onBackendValueChanged: popupDialog.determineActiveColorMode()
}
