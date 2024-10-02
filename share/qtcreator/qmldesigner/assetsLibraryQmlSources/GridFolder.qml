// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioTheme as StudioTheme
import AssetsLibraryBackend

GridItem {
    id: root

    property bool isHighlighted: false

    tooltip.visible: false // disable tooltip

    icon.source: "image://qmldesigner_assets/folder"

    color: root.isHighlighted ? StudioTheme.Values.themeControlBackgroundHover : "transparent"

    mouseArea.onDoubleClicked: {
        assetsView.navToDir(model.filePath)
    }

    DropArea {
        id: dropArea

        anchors.fill: parent

        function updateParentHighlight(highlight) {
            let index = root.assetsView.__modelIndex(root.__currentRow)
            let parentItem = assetsView.__getDelegateParentForIndex(index)
            if (parentItem)
                parentItem.isHighlighted = highlight

            // highlights the root folder canvas area when dragging over child
            if (root.depth === 1 && !root.__isDirectory)
                root.assetsRoot.highlightCanvas = highlight
        }

        onEntered: (drag) => {
            root.assetsRoot.updateDropExtFiles(drag)

            drag.accepted |= (drag.formats[0] === "application/vnd.qtdesignstudio.assets")

            root.isHighlighted = drag.accepted
        }

        onDropped: (drag) => {
            if (drag.formats[0] === "application/vnd.qtdesignstudio.assets") {
                root.rootView.invokeAssetsDrop(drag.urls, model.filePath)
            } else {
                root.rootView.emitExtFilesDrop(root.assetsRoot.dropSimpleExtFiles,
                                               root.assetsRoot.dropComplexExtFiles,
                                               model.filePath)
            }

            root.isHighlighted = false
        }

        onExited: {
            root.isHighlighted = false
        }
    }

    // TODO: mark empty folders
    // Text {
    //     text: qsTr("Empty")
    //     color: "#000000"
    //     font.pixelSize: StudioTheme.Values.smallFontSize
    //     anchors.centerIn: parent
    //     visible: AssetsLibraryBackend.isDirEmpty(model.filePath)
    // }
}
