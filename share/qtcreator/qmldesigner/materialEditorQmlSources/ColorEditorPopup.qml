// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls

StudioControls.PopupDialog {
    id: colorPopup

    property QtObject loaderItem: loader.item
    property color originalColor
    required property color currentColor

    signal activateColor(color : color)

    width: 260

    onOriginalColorChanged: loader.updateOriginalColor()
    onClosing: loader.active = false

    function open(showItem) {
        loader.ensureActive()
        colorPopup.show(showItem)

        loader.updateOriginalColor()
    }

    Loader {
        id: loader

        function ensureActive() {
            if (!loader.active)
                loader.active = true
        }

        function updateOriginalColor() {
            if (loader.status === Loader.Ready)
                loader.item.originalColor = colorPopup.originalColor
        }

        sourceComponent: StudioControls.ColorEditorPopup {
            width: colorPopup.contentWidth
            visible: colorPopup.visible

            onActivateColor: (color) => {
                colorPopup.activateColor(color)
            }
        }

        Binding {
            target: loader.item
            property: "color"
            value: colorPopup.currentColor
            when: loader.status === Loader.Ready
        }

        onLoaded: {
            loader.updateOriginalColor()
            colorPopup.titleBar = loader.item.titleBarContent
        }
    }
}
