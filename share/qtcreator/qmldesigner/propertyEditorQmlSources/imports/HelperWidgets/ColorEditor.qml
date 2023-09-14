// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
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
        if (!colorEditor.backendValue || !colorEditor.backendValue.value)
            return "white" // default color for Rectangle

        if (colorEditor.isVector3D) {
            return Qt.rgba(colorEditor.backendValue.value.x,
                           colorEditor.backendValue.value.y,
                           colorEditor.backendValue.value.z, 1)
        }

        return colorEditor.backendValue.value
    }

    property alias gradientPropertyName: popupLoader.gradientPropertyName

    property alias gradientThumbnail: gradientThumbnail
    property alias shapeGradientThumbnail: shapeGradientThumbnail

    property alias showExtendedFunctionButton: hexTextField.showExtendedFunctionButton

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
            if (popupLoader.isNotInGradientMode())
                colorEditor.syncColor()
        }

        function onBackendValueChanged() {
            if (popupLoader.isNotInGradientMode())
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

        if (!popupLoader.isInValidState)
            return

        popupLoader.commitToGradient()

        // Delay setting the color to keep ui responsive
        if (popupLoader.isNotInGradientMode())
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
            visible: !popupLoader.isNotInGradientMode()
                     && !colorEditor.shapeGradients
                     && popupLoader.hasLinearGradient()
        }

        Shape {
            id: shape
            anchors.fill: parent
            anchors.margins: StudioTheme.Values.border
            visible: !popupLoader.isNotInGradientMode()
                     && colorEditor.shapeGradients

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
                popupLoader.opened ? popupLoader.close() : popupLoader.open()
                forceActiveFocus()
            }
        }

        QtObject {
            id: popupLoader

            property bool isInValidState: popupLoader.active ? popupLoader.dialog.isInValidState : true

            property QtObject dialog: popupLoader.loader.item

            property bool opened: popupLoader.active ? popupLoader.dialog.opened : false

            property string gradientPropertyName

            function commitToGradient() {
                if (!popupLoader.active)
                    return

                if (colorEditor.supportGradient && popupLoader.dialog.gradientModel.hasGradient) {
                    var hexColor = convertColorToString(colorEditor.color)
                    hexTextField.text = hexColor
                    popupLoader.dialog.commitGradientColor()
                }
            }

            function isNotInGradientMode() {
                 if (!popupLoader.active)
                     return true
                 return popupLoader.dialog.isNotInGradientMode()
            }

            function hasLinearGradient(){
                if (!popupLoader.active)
                    return false
                return popupLoader.dialog.hasLinearGradient()
            }

            function ensureLoader() {
                if (!popupLoader.active)
                    popupLoader.active = true
            }

            function open() {
                popupLoader.ensureLoader()
                popupLoader.dialog.open()
            }

            function close() {
                popupLoader.ensureLoader()
                popupLoader.dialog.close()
            }

            function determineActiveColorMode() {
                if (popupLoader.active && popupLoader.dialog)
                    popupLoader.dialog.determineActiveColorMode()
                else
                    colorEditor.syncColor()
            }

            property alias active: popupLoader.loader.active
            property Loader loader: Loader {
                parent: preview
                active: colorEditor.supportGradient

                sourceComponent: ColorEditorPopup {
                    id: cePopup
                    x: cePopup.__defaultX
                    y: cePopup.__defaultY
                }
                onLoaded: popupLoader.dialog.initEditor()
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
        width: implicitWidth
        enabled: popupLoader.isNotInGradientMode()
        writeValueManually: true
        validator: RegExpValidator {
            regExp: /#[0-9A-Fa-f]{6}([0-9A-Fa-f]{2})?/g
        }
        showTranslateCheckBox: false
        backendValue: colorEditor.backendValue

        onAccepted: colorEditor.color = colorFromString(hexTextField.text)
        onCommitData: {
            colorEditor.color = colorFromString(hexTextField.text)
            if (popupLoader.isNotInGradientMode()) {
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

    Component.onCompleted: popupLoader.determineActiveColorMode()

    onBackendValueChanged: {
        popupLoader.determineActiveColorMode()
    }
}
