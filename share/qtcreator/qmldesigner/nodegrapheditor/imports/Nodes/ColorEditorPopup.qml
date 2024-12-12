// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

import HelperWidgets as HelperWidgets
import StudioControls as StudioControls

StudioControls.PopupDialog {
    id: colorPopup

    required property color currentColor
    property QtObject loaderItem: loader.item
    property color originalColor

    signal activateColor(color: color)

    function open(showItem) {
        loader.ensureActive();
        colorPopup.show(showItem);

        loader.updateOriginalColor();
    }

    width: 260

    onClosing: loader.active = false
    onOriginalColorChanged: loader.updateOriginalColor()

    Loader {
        id: loader

        function ensureActive() {
            if (!loader.active)
                loader.active = true;
        }

        function updateOriginalColor() {
            if (loader.status === Loader.Ready)
                loader.item.originalColor = colorPopup.originalColor;
        }

        sourceComponent: StudioControls.ColorEditorPopup {
            visible: colorPopup.visible
            width: colorPopup.contentWidth

            onActivateColor: color => {
                colorPopup.activateColor(color);
            }
        }

        onLoaded: {
            loader.updateOriginalColor();
            colorPopup.titleBar = loader.item.titleBarContent;
        }

        Binding {
            property: "color"
            target: loader.item
            value: colorPopup.currentColor
            when: loader.status === Loader.Ready
        }
    }
}
