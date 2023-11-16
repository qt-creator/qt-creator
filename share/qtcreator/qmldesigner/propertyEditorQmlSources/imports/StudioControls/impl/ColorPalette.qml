// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import QtQuickDesignerColorPalette

Column {
    id: root

    property color selectedColor
    property color oldColor

    property alias enableSingletonConnection: singletonConnection.enabled

    property real twoColumnWidth: 50
    property real fourColumnWidth: 25

    spacing: StudioTheme.Values.colorEditorPopupSpacing

    function addColorToPalette(colorStr) {
        ColorPaletteBackend.addRecentColor(colorStr)
    }

    function showColorDialog(color) {
        root.oldColor = color
        ColorPaletteBackend.showDialog(color)
    }

    signal dialogColorChanged

    Component {
        id: colorItemDelegate

        Rectangle {
            id: colorRectangle

            width: root.fourColumnWidth
            height: StudioTheme.Values.defaultControlHeight
            color: (modelData !== "") ? modelData : "transparent"
            border.color: (modelData !== "") ? StudioTheme.Values.themeControlOutline
                                             : StudioTheme.Values.themeControlOutlineDisabled
            border.width: StudioTheme.Values.border

            Image {
                visible: modelData !== ""
                anchors.fill: parent
                source: "qrc:/navigator/icon/checkers.png"
                fillMode: Image.Tile
                z: -1
            }

            ToolTipArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                tooltip: modelData

                onClicked: function(mouse) {
                    if (mouse.button === Qt.LeftButton)
                        root.selectedColor = colorRectangle.color

                    if (mouse.button === Qt.RightButton && modelData !== "")
                        contextMenu.popup()
                }
            }

            StudioControls.Menu {
                id: contextMenu

                StudioControls.MenuItem {
                    visible: colorMode.currentText === "Favorite"
                    text: qsTr("Remove from Favorites")
                    onTriggered: ColorPaletteBackend.removeFavoriteColor(index)
                    height: visible ? implicitHeight : 0
                }

                StudioControls.MenuItem {
                    visible: colorMode.currentText !== "Favorite"
                    text: qsTr("Add to Favorites")
                    onTriggered: ColorPaletteBackend.addFavoriteColor(modelData)
                    height: visible ? implicitHeight : 0
                }
            }
        }
    }

    Connections {
        id: singletonConnection
        target: ColorPaletteBackend

        function onCurrentColorChanged(color) {
            root.selectedColor = color
            root.dialogColorChanged()
        }

        function onColorDialogRejected() {
            root.selectedColor = root.oldColor
            root.dialogColorChanged()
        }
    }

    StudioControls.ComboBox {
        id: colorMode
        implicitWidth: root.width
        width: colorMode.implicitWidth
        actionIndicatorVisible: false
        model: ColorPaletteBackend.palettes
        currentIndex: colorMode.find(ColorPaletteBackend.currentPalette)

        onActivated: ColorPaletteBackend.currentPalette = colorMode.currentText

        Component.onCompleted: colorMode.currentIndex = colorMode.find(ColorPaletteBackend.currentPalette)
    }

    Grid {
        id: colorPaletteView
        columns: 4
        spacing: StudioTheme.Values.controlGap

        Repeater {
            model: ColorPaletteBackend.currentPaletteColors
            delegate: colorItemDelegate
        }
    }
}
