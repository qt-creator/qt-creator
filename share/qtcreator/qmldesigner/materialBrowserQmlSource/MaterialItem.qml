// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme
import MaterialBrowserBackend

Rectangle {
    id: root

    signal showContextMenu()

    function refreshPreview()
    {
        img.source = ""
        img.source = "image://materialBrowser/" + materialInternalId
    }

    function forceFinishEditing()
    {
        matName.commitRename()
    }

    function startRename()
    {
        matName.startRename()
    }

    border.width: MaterialBrowserBackend.materialBrowserModel.selectedIndex === index ? MaterialBrowserBackend.rootView.materialSectionFocused ? 3 : 1 : 0
    border.color: MaterialBrowserBackend.materialBrowserModel.selectedIndex === index
                        ? StudioTheme.Values.themeControlOutlineInteraction
                        : "transparent"
    color: "transparent"
    visible: materialVisible

    DropArea {
        anchors.fill: parent

        onEntered: (drag) => {
            drag.accepted = drag.formats[0] === "application/vnd.qtdesignstudio.texture"
                         || drag.formats[0] === "application/vnd.qtdesignstudio.bundletexture"
                         || (drag.formats[0] === "application/vnd.qtdesignstudio.assets"
                             && rootView.hasAcceptableAssets(drag.urls))
        }

        onDropped: (drag) => {
            drag.accept()

            if (drag.formats[0] === "application/vnd.qtdesignstudio.texture")
                MaterialBrowserBackend.rootView.acceptTextureDropOnMaterial(index, drag.getDataAsString(drag.keys[0]))
            else if (drag.formats[0] === "application/vnd.qtdesignstudio.bundletexture")
                MaterialBrowserBackend.rootView.acceptBundleTextureDropOnMaterial(index, drag.urls[0])
            else if (drag.formats[0] === "application/vnd.qtdesignstudio.assets")
                MaterialBrowserBackend.rootView.acceptAssetsDropOnMaterial(index, drag.urls)
        }
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onPressed: (mouse) => {
            MaterialBrowserBackend.materialBrowserModel.selectMaterial(index)
            MaterialBrowserBackend.rootView.focusMaterialSection(true)

            if (mouse.button === Qt.LeftButton)
                MaterialBrowserBackend.rootView.startDragMaterial(index, mapToGlobal(mouse.x, mouse.y))
            else if (mouse.button === Qt.RightButton)
                root.showContextMenu()
        }

        onDoubleClicked: MaterialBrowserBackend.materialBrowserModel.openMaterialEditor();
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

        // Eat keys so they are not passed to parent while editing name
        Keys.onPressed: (e) => {
            e.accepted = true;
        }

        MaterialBrowserItemName {
            id: matName

            text: materialName
            width: img.width
            anchors.horizontalCenter: parent.horizontalCenter

            onRenamed: (newName) => {
                MaterialBrowserBackend.materialBrowserModel.renameMaterial(index, newName);
                mouseArea.forceActiveFocus()
            }

            onClicked: {
                MaterialBrowserBackend.materialBrowserModel.selectMaterial(index)
                MaterialBrowserBackend.rootView.focusMaterialSection(true)
            }
        }
    }
}
