// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuick.Shapes
import QtQuick.Templates as T
import HelperWidgets 2.0 as HelperWidgets
import StudioTheme as StudioTheme
import StudioControls as StudioControls
import QtQuickDesignerTheme
import QtQuickDesignerColorPalette

Row {
    id: colorEditor

    property color color
    property bool supportGradient: false
    readonly property color __editColor: edit

    property variant value: {
        if (!edit)
            return "white" // default color for Rectangle

        if (colorEditor.isVector3D) {
            return Qt.rgba(__editColor.x,
                           __editColor.y,
                           __editColor.z, 1)
        }

        return __editColor
    }

    property alias gradientPropertyName: popupDialog.gradientPropertyName

    property alias gradientThumbnail: gradientThumbnail
    property alias shapeGradientThumbnail: shapeGradientThumbnail

    property bool shapeGradients: false
    property bool isVector3D: false
    property color originalColor

    property bool __block: false

    function resetShapeColor() {
        if (edit)
            edit = ""
    }

    function writeColor() {
        if (colorEditor.isVector3D) {
            edit = Qt.vector3d(colorEditor.color.r,
                               colorEditor.color.g,
                               colorEditor.color.b)
        } else {
            edit = colorEditor.color
        }
    }

    function initEditor() {
        colorEditor.syncColor()
    }

    // Syncing color from backend to frontend and block reflection
    function syncColor() {
        colorEditor.__block = true
        colorEditor.color = colorEditor.value
        hexTextField.syncColor()
        colorEditor.__block = false
    }

    Connections {
        id: backendConnection

        target: colorEditor

        function onValueChanged() {
            if (popupDialog.isSolid())
                colorEditor.syncColor()
        }

        function on__EditColorChanged() {
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
            colorEditor.writeColor()
            hexTextField.syncColor()
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
            source: "qrc:/navigator/icon/checkers.png"
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
                    edit = hexColor
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

                sourceComponent: HelperWidgets.ColorEditorPopup {
                    shapeGradients: colorEditor.shapeGradients
                    supportGradient: colorEditor.supportGradient
                    width: popupDialog.contentWidth
                }

                onLoaded: {
                    popupDialog.loaderItem.initEditor()
                    popupDialog.titleBar = loader.item.titleBarContent
                }
            }
        }
    }

    HelperWidgets.LineEdit {
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
        showExtendedFunctionButton: false
        indicatorVisible: false

        onAccepted: colorEditor.color = hexTextField.text
        onCommitData: {
            colorEditor.color = hexTextField.text
            if (popupDialog.isSolid())
                colorEditor.writeColor()
        }

        function syncColor() {
            hexTextField.text = colorEditor.color
        }
    }

    Component.onCompleted: popupDialog.determineActiveColorMode()

    on__EditColorChanged: popupDialog.determineActiveColorMode()
}
