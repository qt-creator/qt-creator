// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
        matName.ensureVisible(matName.text.length)
        nameMouseArea.enabled = false
    }

    function commitRename()
    {
        if (matName.readOnly)
            return;

        materialBrowserModel.renameMaterial(index, matName.text);
        mouseArea.forceActiveFocus()
    }

    border.width: materialBrowserModel.selectedIndex === index ? rootView.materialSectionFocused ? 3 : 1 : 0
    border.color: materialBrowserModel.selectedIndex === index
                        ? StudioTheme.Values.themeControlOutlineInteraction
                        : "transparent"
    color: "transparent"
    visible: materialVisible

    DropArea {
        anchors.fill: parent

        onEntered: (drag) => {
            drag.accepted = drag.formats[0] === "application/vnd.qtdesignstudio.texture"
        }

        onDropped: (drag) => {
            rootView.acceptTextureDropOnMaterial(index, drag.getDataAsString(drag.keys[0]))
        }
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onPressed: (mouse) => {
            rootView.focusMaterialSection(true)
            materialBrowserModel.selectMaterial(index)

            if (mouse.button === Qt.LeftButton)
                rootView.startDragMaterial(index, mapToGlobal(mouse.x, mouse.y))
            else if (mouse.button === Qt.RightButton)
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

            onActiveFocusChanged: {
                if (!activeFocus) {
                    matName.readOnly = true
                    nameMouseArea.enabled = true
                    ensureVisible(0)
                }
            }

            Component.onCompleted: ensureVisible(0)

            MouseArea {
                id: nameMouseArea

                anchors.fill: parent

                onClicked: {
                    rootView.focusMaterialSection(true)
                    materialBrowserModel.selectMaterial(index)
                }
                onDoubleClicked: root.startRename()
            }
        }
    }
}
