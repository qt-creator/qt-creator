// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioTheme as StudioTheme
import AssetsLibraryBackend

GridItem {
    id: root

    readonly property string suffix: model.fileName.substr(-4)
    readonly property bool isFont: root.suffix === ".ttf" || root.suffix === ".otf"
    readonly property bool isEffect: root.suffix === ".qep"

    icon.source: "image://qmldesigner_assets/" + model.filePath

    mouseArea.onPressed: (mouse) => {
        var ctrlDown = mouse.modifiers & Qt.ControlModifier
        if (mouse.button === Qt.LeftButton) {
           if (!root.assetsView.isAssetSelected(model.filePath) && !ctrlDown)
               root.assetsView.clearSelectedAssets()
           root.isSelected = ctrlDown ? !root.assetsView.isAssetSelected(model.filePath) : true
           root.assetsView.setAssetSelected(model.filePath, root.isSelected)

           if (root.isSelected) {
               let selectedPaths = root.assetsView.selectedPathsAsList()
               root.rootView.startDragAsset(selectedPaths, mapToGlobal(mouse.x, mouse.y))
           }
        } else {
           if (!root.assetsView.isAssetSelected(model.filePath) && !ctrlDown)
               root.assetsView.clearSelectedAssets()
           root.isSelected = root.assetsView.isAssetSelected(model.filePath) || !ctrlDown
           root.assetsView.setAssetSelected(model.filePath, root.isSelected)
        }
    }

    mouseArea.onReleased: (mouse) => {
        if (mouse.button === Qt.LeftButton) {
            if (!(mouse.modifiers & Qt.ControlModifier))
                root.assetsView.selectedAssets = {}
            root.assetsView.selectedAssets[model.filePath] = root.isSelected
            root.assetsView.updateSelectedAssets()

            root.assetsView.currentFilePath = model.filePath
        }
    }

    mouseArea.onDoubleClicked: (mouse) => {
        if (mouse.button === Qt.LeftButton && root.isEffect)
            root.rootView.openEffectComposer(filePath)
    }

    mouseArea.onClicked: (mouse) => {
        if (mouse.button === Qt.RightButton) {
            function onFolderCreated(path) {
                if (path)
                    root.assetsView.addCreatedFolder(path)
            }

            let parentDirIndex = assetsModel.parentDirIndex(model.filePath)
            let selectedPaths = root.assetsView.selectedPathsAsList()
            let modelIndex = root.assetsModel.indexForPath(model.filePath)
            root.assetsView.contextMenu.openContextMenuForFile(modelIndex, parentDirIndex,
            selectedPaths, onFolderCreated)
        }
    }

    tooltip.text: {
        let filePath = model.filePath.replace(root.assetsModel.contentDirPath(), "")
        let fileSize = root.rootView.assetFileSize(model.filePath)
        let fileExtMatches = model.filePath.match(/\.(.*)$/)
        let fileExt = fileExtMatches ? "(" + fileExtMatches[1] + ")" : ""

        if (root.rootView.assetIsImageOrTexture(model.filePath)) {
            let size = root.rootView.imageSize(model.filePath)

            return filePath + "\n"
                    + size.width + " x " + size.height
                    + "\n" + fileSize
                    + " " + fileExt
        } else {
            return filePath + "\n"
                    + fileSize
                    + " " + fileExt
        }
    }
}
