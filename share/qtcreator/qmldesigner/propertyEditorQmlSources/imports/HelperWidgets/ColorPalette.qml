// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme
import QtQuickDesignerColorPalette 1.0

Column {
    id: root

    property color selectedColor
    property color oldColor

    property alias enableSingletonConnection: singletonConnection.enabled

    spacing: 10

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

            width: StudioTheme.Values.colorEditorPopupSpinBoxWidth
            height: StudioTheme.Values.defaultControlHeight
            color: (modelData !== "") ? modelData : "transparent"
            border.color: (modelData !== "") ? StudioTheme.Values.themeControlOutline
                                             : StudioTheme.Values.themeControlOutlineDisabled
            border.width: StudioTheme.Values.border

            Image {
                visible: modelData !== ""
                anchors.fill: parent
                source: "images/checkers.png"
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
            dialogColorChanged()
        }

        function onColorDialogRejected() {
            root.selectedColor = root.oldColor
            dialogColorChanged()
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
            model: ColorPaletteBackend.palettes
            currentIndex: colorMode.find(ColorPaletteBackend.currentPalette)

            onActivated: ColorPaletteBackend.currentPalette = colorMode.currentText

            Component.onCompleted: colorMode.currentIndex = colorMode.find(ColorPaletteBackend.currentPalette)
        }
    }

    GridView {
        id: colorPaletteView
        model: ColorPaletteBackend.currentPaletteColors
        delegate: colorItemDelegate
        cellWidth: StudioTheme.Values.colorEditorPopupSpinBoxWidth
                   + StudioTheme.Values.controlGap
        cellHeight: StudioTheme.Values.defaultControlHeight
                    + StudioTheme.Values.controlGap
        width: 4 * (StudioTheme.Values.colorEditorPopupSpinBoxWidth
                    + StudioTheme.Values.controlGap)
        height: 2 * (StudioTheme.Values.defaultControlHeight
                     + StudioTheme.Values.controlGap)
        clip: true
        interactive: false
    }
}
