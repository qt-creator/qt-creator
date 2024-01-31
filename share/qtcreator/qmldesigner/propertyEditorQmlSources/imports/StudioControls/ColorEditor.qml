// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioTheme as StudioTheme
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

    width: root.style.controlSize.width
    height: root.style.controlSize.height

    ColorBackend {
        id: colorBackend
    }

    ActionIndicator {
        id: actionIndicator
        style: root.style
        __parentControl: root
        x: 0
        y: 0
        width: actionIndicator.visible ? root.__actionIndicatorWidth : 0
        height: actionIndicator.visible ? root.__actionIndicatorHeight : 0
    }

    Rectangle {
        id: preview
        x: actionIndicator.width
        y: 0
        z: previewMouseArea.containsMouse ? 10 : 0
        implicitWidth: root.style.controlSize.height
        implicitHeight: root.style.controlSize.height
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
                popupDialog.visibility ? popupDialog.close() : popupDialog.open()
                previewMouseArea.forceActiveFocus()
            }
        }

        PopupDialog {
            id: popupDialog

            property QtObject loaderItem: loader.item

            width: 260

            function ensureLoader() {
                if (!loader.active)
                    loader.active = true
            }

            function open() {
                popupDialog.ensureLoader()
                popupDialog.show(preview)

                if (loader.status === Loader.Ready)
                    loader.item.originalColor = root.color
            }

            Loader {
                id: loader

                sourceComponent: ColorEditorPopup {
                    id: popup
                    width: popupDialog.contentWidth
                    visible: popupDialog.visible

                    onActivateColor: function(color) {
                        colorBackend.activateColor(color)
                    }
                }

                Binding {
                    target: loader.item
                    property: "color"
                    value: root.color
                    when: loader.status === Loader.Ready
                }

                onLoaded: {
                    loader.item.originalColor = root.color
                    popupDialog.titleBar = loader.item.titleBarContent
                }
            }
        }
    }

    TextField {
        id: hexTextField
        style: root.style
        x: actionIndicator.width + preview.width - preview.border.width
        y: 0
        width: root.width - hexTextField.x
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
