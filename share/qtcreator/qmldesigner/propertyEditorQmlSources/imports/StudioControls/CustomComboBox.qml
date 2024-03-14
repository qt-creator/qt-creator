// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

// A ComboBox with a custom popup window
StudioControls.ComboBox {
    id: root

    required property Component popupComponent

    readonly property int popupHeight: Math.min(800, popupLoader.height + 2)

    property alias popupItem: popupLoader.item

    // hide default popup
    popup.width: 0
    popup.height: 0

    function calculateWindowGeometry() {
        let gPos = globalPos(itemPane.mapFromItem(root, 0, 0))
        let scrRect = screenRect()

        window.width = Math.max(root.width - root.actionIndicator.width, popupLoader.width + 2) // 2: scrollView left and right 1px margins

        let newX = gPos.x + root.width - window.width
        if (newX < scrRect.x)
            newX = gPos.x

        let newY = Math.min(scrRect.y + scrRect.height,
                            Math.max(scrRect.y, gPos.y + root.height - 1))

        // Check if we have more space above or below the control, and put control on that side,
        // unless we have enough room for maximum size popup under the control
        let newHeight
        let screenY = newY - scrRect.y
        let availableHeight = scrRect.height - screenY
        if (availableHeight > screenY || availableHeight > root.popupHeight) {
            newHeight = Math.min(root.popupHeight, availableHeight)
        } else {
            newHeight = Math.min(root.popupHeight, screenY - root.height)
            newY = newY - newHeight - root.height + 1
        }

        window.height = newHeight
        window.x = newX
        window.y = newY
    }

    Connections {
        target: root.popup

        function onAboutToShow() {
            root.calculateWindowGeometry()

            window.show()
            window.requestActivate()

            // Geometry can get corrupted by first show after screen change, so recalc it
            root.calculateWindowGeometry()
        }

        function onAboutToHide() {
            window.hide()
        }
    }

    Connections {
        target: itemPane.scrollView

        function onContentYChanged() {
            window.hide() // TODO: a better solution is to move the window instead of hiding
        }
    }

    Window {
        id: window

        flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint

        onActiveFocusItemChanged: {
            if (!window.activeFocusItem && !root.hovered && root.popup.opened)
                root.popup.close()
        }

        Rectangle {
            anchors.fill: parent
            color: StudioTheme.Values.themePanelBackground
            border.color: StudioTheme.Values.themeInteraction
            border.width: 1
            focus: true

            HelperWidgets.ScrollView {
                anchors.fill: parent
                anchors.margins: 1

                Loader {
                    id: popupLoader
                    sourceComponent: root.popupComponent
                }
            }

            Keys.onPressed: function(event) {
                if (event.key === Qt.Key_Escape && root.popup.opened)
                    root.popup.close()
            }
        }
    }
}
