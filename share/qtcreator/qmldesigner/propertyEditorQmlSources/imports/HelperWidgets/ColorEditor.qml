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
        if (colorEditor.backendValue === undefined
                || colorEditor.backendValue.value === undefined)
            return "white" // default color for Rectangle

        if (colorEditor.isVector3D)
            return Qt.rgba(colorEditor.backendValue.value.x,
                           colorEditor.backendValue.value.y,
                           colorEditor.backendValue.value.z, 1)
        else
            return colorEditor.backendValue.value
    }
    property alias gradientPropertyName: popupLoader.gradientPropertyName

    property alias gradientThumbnail: gradientThumbnail
    property alias shapeGradientThumbnail: shapeGradientThumbnail

    property bool shapeGradients: false
    property color originalColor
    property bool isVector3D: false

    property alias spacer: spacer

    function resetShapeColor() {
        colorEditor.backendValue.resetValue()
    }

    function initEditor() {
        colorEditor.color = colorEditor.value
    }

    Connections {
        id: backendConnection
        target: colorEditor

        function onValueChanged() {
            if (popupLoader.isNotInGradientMode())
                colorEditor.color = colorEditor.value
        }

        function onBackendValueChanged() {
            if (popupLoader.isNotInGradientMode())
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
                    colorEditor.color = colorEditor.value
            }

            property alias active: popupLoader.loader.active
            property Loader loader: Loader {
                parent: colorEditor
                active: colorEditor.supportGradient
                sourceComponent: ColorEditorPopup {
                    id: cePopup
                    x: cePopup.__defaultX
                    y: cePopup.__defaultY
                }
                onLoaded: {
                    popupLoader.dialog.initEditor()
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
