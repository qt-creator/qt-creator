// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Window
import StudioTheme as StudioTheme
import StudioQuickUtils 1.0

/*
    PopupPanel is a floating window that aligns itself to one side of a reference item (snapItem)
    based on the available space around the snapItem.
*/
Window {
    id: root

    enum VerticalDirection { BottomToTop, TopToBottom }

    // The window would be snapped to snapItem
    required property Item snapItem

    property int implicitWidth: 400
    property int implicitHeight: 1000

    // The window is preferred to be snapped to this corner of the snapItem
    property int preferredCorner: Qt.AlignBottom | Qt.AlignLeft
    property int horizontalDirection: Qt.LeftToRight
    property int verticalDirection: PopupPanel.TopToBottom

    // Available size for the popup.
    // This might be larger than the window size, but it's limited to the screen size and size boundaries of root
    readonly property alias availableSize: background.availableSize

    visible: false
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint

    onActiveFocusItemChanged: {
        if (!root.activeFocusItem)
            root.close()
    }

    onImplicitWidthChanged: {
        if (root.visible)
            root.calculateWindowGeometry()
    }

    onImplicitHeightChanged: {
        if (root.visible)
            root.calculateWindowGeometry()
    }

    function showPanel() {
        root.show()
        root.requestActivate()

        root.width = 0
        root.height = 0

        root.calculateWindowGeometry()
    }

    function calculateWindowGeometry() {
        let refRect = root.globalRect(root.snapItem)
        let screenRect = Utils.screenContaining(refRect.x, refRect.y)
        let maxSize = root.getMaxAvailableSize(screenRect, refRect)

        let x = refRect.x
        let y = refRect.y
        let width = Math.min(root.implicitWidth, maxSize.width)
        let height = Math.min(root.implicitHeight, maxSize.height)
        let justifiedSize = getJustifiedSize(width, height, root)
        width = justifiedSize.width
        height = justifiedSize.height

        // Apply corner offset
        if (preferredCorner & Qt.AlignRight)
            x += refRect.width
        if (preferredCorner & Qt.AlignBottom)
            y += refRect.height

        // Determine effective direction
        let effectiveVertical = verticalDirection
        let effectiveHorizontal = horizontalDirection

        let predictedX = x
        let predictedY = y

        if (horizontalDirection === Qt.RightToLeft)
            predictedX -= width
        if (verticalDirection === PopupPanel.BottomToTop)
            predictedY -= height

        if (predictedY + height > screenRect.y + screenRect.height || predictedY < screenRect.y)
            effectiveVertical = (verticalDirection === PopupPanel.BottomToTop) ? PopupPanel.TopToBottom : PopupPanel.BottomToTop

        if (predictedX + width > screenRect.x + screenRect.width || predictedX < screenRect.x)
            effectiveHorizontal = (horizontalDirection === Qt.RightToLeft) ? Qt.LeftToRight : Qt.RightToLeft

        if (effectiveHorizontal === Qt.RightToLeft)
            x -= width

        if (effectiveVertical === PopupPanel.BottomToTop)
            y -= height

        // Handle overlaps on the snapItem
        if (preferredCorner & Qt.AlignRight && effectiveHorizontal === Qt.RightToLeft) {
            if (preferredCorner & Qt.AlignTop && effectiveVertical === PopupPanel.TopToBottom) {
                if (effectiveHorizontal != horizontalDirection)
                    x -= refRect.width
                else
                    y += refRect.height
            } else if (preferredCorner & Qt.AlignBottom && effectiveVertical === PopupPanel.BottomToTop) {
                if (effectiveHorizontal != horizontalDirection)
                    x -= refRect.width
                else
                    y -= refRect.height
            }
        }

        if (preferredCorner & Qt.AlignLeft && effectiveHorizontal === Qt.LeftToRight) {
            if (preferredCorner & Qt.AlignTop && effectiveVertical === PopupPanel.TopToBottom) {
                if (effectiveVertical != verticalDirection)
                    y += refRect.height
                else
                    x += refRect.width
            } else if (preferredCorner & Qt.AlignBottom && effectiveVertical === PopupPanel.BottomToTop) {
                if (effectiveVertical != verticalDirection)
                    y -= refRect.height
                else
                    x += refRect.width
            }
        }

        x = Math.max(screenRect.x, Math.min(x, screenRect.x + screenRect.width - width))
        y = Math.max(screenRect.y, Math.min(y, screenRect.y + screenRect.height - height))

        root.setGeometry(x, y, width, height)
        background.calcAvailableSize(screenRect)
    }

    function globalRect(item : Item) : rect {
        let refItem = item.parent ?? item
        let topLeft = refItem.mapToGlobal(item.x, item.y)
        return Qt.rect(topLeft.x, topLeft.y, item.width, item.height)
    }

    function getMaxAvailableSize(screen : rect, snapRect : rect) : size {
        let maxW = Math.max(snapRect.left - screen.left, screen.right - snapRect.right)
        let maxH = Math.max(snapRect.top - screen.top, screen.bottom - snapRect.bottom)
        return Qt.size(maxW, maxH)
    }

    function getJustifiedSize(width: int, height: int, refItem : var) : size {
        if (refItem.minimumWidth && refItem.minimumWidth > -1)
            width = Math.max(width, refItem.minimumWidth)

        if (refItem.minimumHeight && refItem.minimumHeight > -1)
            height = Math.max(height, refItem.minimumHeight)

        if (refItem.maximumWidth && refItem.maximumWidth > -1)
            width = Math.min(width, refItem.maximumWidth)

        if (refItem.maximumHeight && refItem.maximumHeight > -1)
            height = Math.min(height, refItem.maximumHeight)

        return Qt.size(width, height)
    }

    Rectangle {
        id: background

        property size availableSize

        function calcAvailableSize(screen : rect) {
            if (!root.snapItem) {
                background.availableSize = Qt.size(screen.width, screen.height)
                return
            }

            let bgRect = root.globalRect(background)
            let refRect = root.globalRect(root.snapItem)
            let maxRect = {}

            // Calculate maxRect
            if (bgRect.right <= refRect.left) {
                maxRect.left = screen.left
                maxRect.right = refRect.left
            } else if (bgRect.right <= refRect.right) {
                maxRect.left = screen.left
                maxRect.right = refRect.right
            } else if (bgRect.left >= refRect.right) {
                maxRect.left = refRect.right
                maxRect.right = screen.right
            } else {
                maxRect.left = refRect.left
                maxRect.right = screen.right
            }

            if (bgRect.bottom <= refRect.top) {
                maxRect.top = screen.top
                maxRect.bottom = refRect.top
            } else if (bgRect.bottom <= refRect.bottom) {
                maxRect.top = screen.top
                maxRect.bottom = refRect.bottom
            } else if (bgRect.top >= refRect.bottom) {
                maxRect.top = refRect.bottom
                maxRect.bottom = screen.bottom
            } else {
                maxRect.top = refRect.top
                maxRect.bottom = screen.bottom
            }

            let width = maxRect.right - maxRect.left
            let height = maxRect.bottom - maxRect.top

            background.availableSize = root.getJustifiedSize(width, height, root)
        }

        anchors.fill: parent
        color: StudioTheme.Values.themePanelBackground
        border.color: StudioTheme.Values.themeInteraction
        border.width: 1
    }
}
