/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.1
import QtQuick.Controls 2.5
import StudioControls 1.0 as StudioControls
import HelperWidgets 2.0
import QtQuick.Controls.Private 1.0 // showing a ToolTip

Item {
    property color selectedColor
    property bool clickable : true


    width: 200
    height: 40
    enabled: clickable

    function addColorToPalette(colorCode)
    {
        paletteModel.addItem(colorCode)
    }

    Component {
        id: colorItemDelegate


        Rectangle {
            id: backgroundColor
            property var favorite : isFavorite
            height: 27
            width: 27
            border.color: (backgroundColor.favorite ? "#ffd700" : "#555555")
            border.width: (backgroundColor.favorite ? 2 : 1)
            color: "white"
            radius: 0

            Rectangle {
                id: colorRectangle
                width: 25
                height: 25
                anchors.centerIn: parent
                color: colorCode

                border.color: "black"
                border.width: 1
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: {
                    if ((mouse.button === Qt.LeftButton) && clickable)
                        selectedColor = colorRectangle.color
                }
                onPressed: {
                    if (mouse.button === Qt.RightButton)
                        contextMenu.popup()
                }
            }
            StudioControls.Menu {
                id: contextMenu
                StudioControls.MenuItem {
                    text: (backgroundColor.favorite
                           ? qsTr("Remove from Favorites")
                           : qsTr("Add to Favorites"))
                    onTriggered: {
                        paletteModel.toggleFavorite(index)
                    }
                }
            }
        }
    }

    SimpleColorPaletteModel { id: paletteModel }
    ListView {
        id: colorPaletteView
        model: paletteModel
        delegate: colorItemDelegate
        orientation: Qt.Horizontal
        anchors.fill: parent
        clip: true
        interactive: false
        spacing: 2
    }
}
