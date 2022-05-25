/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: root

    signal showContextMenu()

    function refreshPreview()
    {
        img.source = ""
        img.source = "image://materialBrowser/" + materialInternalId
    }

    function startRename()
    {
        matName.readOnly = false
        matName.selectAll()
        matName.forceActiveFocus()
        nameMouseArea.enabled = false
    }

    function commitRename()
    {
        if (matName.readOnly)
            return;

        matName.readOnly = true
        nameMouseArea.enabled = true

        materialBrowserModel.renameMaterial(index, matName.text);
    }

    border.width: materialBrowserModel.selectedIndex === index ? 1 : 0
    border.color: materialBrowserModel.selectedIndex === index
                        ? StudioTheme.Values.themeControlOutlineInteraction
                        : "transparent"
    color: "transparent"
    visible: materialVisible

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onClicked: (mouse) => {
            materialBrowserModel.selectMaterial(index)
            if (mouse.button === Qt.RightButton)
                root.showContextMenu()
        }

        onDoubleClicked: materialBrowserModel.openMaterialEditor();
    }

    Column {
        anchors.fill: parent
        spacing: 1

        Item { width: 1; height: 5 } // spacer

        Image {
            id: img

            width: root.width - 10
            height: img.width
            anchors.horizontalCenter: parent.horizontalCenter
            source: "image://materialBrowser/" + materialInternalId
            cache: false
        }

        TextInput {
            id: matName

            text: materialName

            width: img.width
            clip: true
            anchors.horizontalCenter: parent.horizontalCenter
            horizontalAlignment: TextInput.AlignHCenter

            font.pixelSize: StudioTheme.Values.myFontSize

            readOnly: true
            selectByMouse: !matName.readOnly

            color: StudioTheme.Values.themeTextColor
            selectionColor: StudioTheme.Values.themeTextSelectionColor
            selectedTextColor: StudioTheme.Values.themeTextSelectedTextColor

            // allow only alphanumeric characters, underscores, no space at start, and 1 space between words
            validator: RegExpValidator { regExp: /^(\w+\s)*\w+$/ }

            onEditingFinished: root.commitRename()

            MouseArea {
                id: nameMouseArea

                anchors.fill: parent

                onClicked: materialBrowserModel.selectMaterial(index)
                onDoubleClicked: root.startRename()
            }
        }
    }
}
