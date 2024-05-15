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
import StudioHelpers

Item {
    id: root

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property alias color: colorBackend.color

    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: root.style.actionIndicatorSize.width
    property real __actionIndicatorHeight: root.style.actionIndicatorSize.height

    property alias showHexTextField: hexTextField.visible

    readonly property real padding: 2
    readonly property real innerItemsHeight: root.height - 2 * root.padding

    width: root.style.controlSize.width
    height: root.style.controlSize.height

    clip: true

    signal editorOpened(var item, var editorPopup)

    function closeEditor() {
        loader.close()
    }

    ColorBackend {
        id: colorBackend
    }

    StudioControls.ActionIndicator {
        id: actionIndicator
        style: root.style
        __parentControl: root
        x: root.padding
        y: root.padding
        width: actionIndicator.visible ? root.__actionIndicatorWidth : 0
        height: actionIndicator.visible ? root.__actionIndicatorHeight : 0
    }

    Rectangle {
        id: preview
        x: root.padding + actionIndicator.width
        y: root.padding
        z: previewMouseArea.containsMouse ? 10 : 0
        implicitWidth: root.innerItemsHeight
        implicitHeight: root.innerItemsHeight
        color: root.color
        border.color: previewMouseArea.containsMouse ? root.style.border.hover : root.style.border.idle
        border.width: root.style.borderWidth

        Image {
            anchors.fill: parent
            source: "qrc:/navigator/icon/checkers.png"
            fillMode: Image.Tile
            z: -1
        }

        MouseArea {
            id: previewMouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                loader.toggle()
                previewMouseArea.forceActiveFocus()
            }
        }

        Loader {
            id: loader

            function ensureLoader() {
                if (!loader.active)
                    loader.active = true
                if (loader.sourceComponent === null)
                    loader.sourceComponent = popupDialogComponent
            }

            function open() {
                loader.ensureLoader()
                loader.item.show(preview)

                if (loader.status === Loader.Ready)
                    loader.item.originalColor = root.color
            }

            function close() {
                if (loader.item)
                    loader.item.close()
            }

            function toggle() {
                if (loader.item)
                    loader.close()
                else
                    loader.open()
            }

            Component {
                id: popupDialogComponent


                StudioControls.PopupDialog {
                    id: popupDialog

                    property alias color: popup.color
                    property alias originalColor: popup.originalColor
                    titleBar: popup.titleBarContent

                    width: 260

                    StudioControls.ColorEditorPopup {
                        id: popup
                        width: popupDialog.contentWidth

                        onActivateColor: function(color) {
                            colorBackend.activateColor(color)
                        }
                    }

                    onClosing: {
                        loader.sourceComponent = null
                    }
                }
            }

            sourceComponent: null

            Binding {
                target: loader.item
                property: "color"
                value: root.color
                when: loader.status === Loader.Ready
            }

            onLoaded: {
                loader.item.originalColor = root.color
                root.editorOpened(root, loader.item)
            }
        }
    }

    StudioControls.TextField {
        id: hexTextField
        style: root.style
        x: root.padding + actionIndicator.width + preview.width - preview.border.width
        y: root.padding
        width: root.width - hexTextField.x - 2 * root.padding
        height: root.innerItemsHeight
        text: root.color
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

        onAccepted: colorBackend.activateColor(colorFromString(hexTextField.text))
    }
}
