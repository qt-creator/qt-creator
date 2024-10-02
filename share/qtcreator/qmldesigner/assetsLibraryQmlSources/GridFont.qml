// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioTheme as StudioTheme
import AssetsLibraryBackend

GridFile {
    id: root

    property bool allowTooltip: true

    tooltip.visible: false // disable default tooltip

    mouseArea.onEntered: root.allowTooltip = true
    mouseArea.onExited: AssetsLibraryBackend.tooltipBackend.hideTooltip()
    mouseArea.onPositionChanged: AssetsLibraryBackend.tooltipBackend.reposition()

    mouseArea.onCanceled: {
        AssetsLibraryBackend.tooltipBackend.hideTooltip()
        root.allowTooltip = true
    }

    mouseArea.onPressed: (mouse) => {
        print(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 2")
        root.allowTooltip = false
        AssetsLibraryBackend.tooltipBackend.hideTooltip()
    }

    mouseArea.onReleased: (mouse) => {
        root.allowTooltip = true
    }

    mouseArea.onDoubleClicked: (mouse) => {
        root.allowTooltip = false
        AssetsLibraryBackend.tooltipBackend.hideTooltip()
    }

    Timer {
        interval: 1000
        running: mouseArea.containsMouse && root.allowTooltip

        onTriggered: {
            AssetsLibraryBackend.tooltipBackend.name = model.fileName
            AssetsLibraryBackend.tooltipBackend.path = model.filePath
            AssetsLibraryBackend.tooltipBackend.showTooltip()
        }
    }
}
