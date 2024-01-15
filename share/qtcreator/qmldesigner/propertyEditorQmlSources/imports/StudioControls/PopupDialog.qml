// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls.Basic as Basic
import QtQuick.Window
import QtQuick.Shapes
import StudioTheme as StudioTheme
import StudioWindowManager

QtObject {
    id: root

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property alias titleBar: titleBarContent.children
    default property alias content: mainContent.children
    property alias contentWidth: mainContent.width

    property int width: 320
    property int height: column.implicitHeight
    property int maximumWidth: -1
    property int maximumHeight: -1

    property alias flags: window.flags
    property alias visible: window.visible

    property int anchorGap: 10
    property int edge: Qt.LeftEdge
    property int actualEdge: root.edge
    //property alias chevronVisible: chevron.visible

    property rect __itemGlobal: Qt.rect(0, 0, 100, 100)

    property bool keepOpen: false

    signal closing(close: var)

    function showGlobal() {
        var pos = WindowManager.globalCursorPosition()
        root.__itemGlobal = Qt.rect(pos.x, pos.y, 300, 20)
        //root.chevronVisible = false
        root.layout()
        window.show()
        window.raise()
    }

    function show(target: Item) {
        var originGlobal = target.mapToGlobal(0, 0)
        root.__itemGlobal = Qt.rect(originGlobal.x, originGlobal.y, target.width, target.height)
        //root.chevronVisible = true
        root.layout()
        window.show()
        window.raise()
    }

    function close() {
        window.close()
    }

    function layout() {
        let position = Qt.point(root.__itemGlobal.x, root.__itemGlobal.y)
        var screen = WindowManager.getScreenGeometry(position)

        // Collect region information
        let edges = window.getRegions(screen, root.__itemGlobal)

        if (Object.keys(edges).length === 0) {
            console.log("Warning: Couldn't find regions.")
            return
        }

        // Determine edge
        let edge = window.findPlacement(edges)
        root.actualEdge = edge

        let anchor = edges[edge].anchor
        let popoverRect = window.popoverGeometry(edge, anchor, edges[edge].region)

        //if (chevron.visible)
        //    chevron.layout(edge, popoverRect, anchor)

        window.x = popoverRect.x
        window.y = popoverRect.y
    }

    property Window window: Window {
        id: window

        property int margin: 0 //20

        width: root.width + (2 * window.margin)
        height: root.height + (2 * window.margin)
        maximumWidth: {
            if (root.maximumWidth < 0)
                return root.width + (2 * window.margin)
            else
                return root.maximumWidth + (2 * window.margin)
        }
        maximumHeight:{
            if (root.maximumHeight < 0)
                return root.height + (2 * window.margin)
            else
                return root.maximumHeight + (2 * window.margin)
        }
        visible: false
        flags: Qt.FramelessWindowHint | Qt.Dialog | Qt.WindowStaysOnTopHint
        color: "transparent"

        onClosing: function (close) {
            root.closing(close)
        }

        function findPlacement(edges: var): int {
            if (edges[root.edge].fit) // Original edge does fit
                return root.edge

            return window.findAlternativePlacement(edges)
        }

        function findAlternativePlacement(edges: var): int {
            let horizontal = (Qt.LeftEdge | Qt.RightEdge)
            if (root.edge & horizontal)
                if (edges[root.edge ^ horizontal].fit)
                    return root.edge ^ horizontal

            let vertical = (Qt.TopEdge | Qt.BottomEdge)
            if (root.edge & vertical)
                if (edges[root.edge ^ vertical].fit)
                    return root.edge ^ vertical

            // Take the first that fits
            for (var key in edges) {
                if (edges[key].fit)
                    return Number(key)
            }

            return Qt.LeftEdge // Default
        }

        function contains(a: rect, b: rect): bool {
            let halfSizeA = Qt.size(a.width * 0.5, a.height * 0.5)
            let halfSizeB = Qt.size(b.width * 0.5, b.height * 0.5)

            let centerA = Qt.point(a.x + halfSizeA.width, a.y + halfSizeA.height)
            let centerB = Qt.point(b.x + halfSizeB.width, b.y + halfSizeB.height)

            if (Math.abs(centerA.x - centerB.x) > (halfSizeA.width + halfSizeB.width))
                return false

            if (Math.abs(centerA.y - centerB.y) > (halfSizeA.height + halfSizeB.height))
                return false

            return true
        }

        function getRegions(source: rect, target: rect) {
            var edges = {}

            // Overlaps or Inside
            if (!window.contains(source, target))
                return edges

            var targetCenter = Qt.point(target.x + (target.width * 0.5),
                                        target.y + (target.height * 0.5))

            // Just as a reminder why calculating custom right and bottom:
            // > Note that for historical reasons this function returns top() + height() - 1;
            // > use y() + height() to retrieve the true y-coordinate.
            let sourceRight = source.x + source.width
            let sourceBottom = source.y + source.height

            // TOP
            let topAnchor = Qt.point(targetCenter.x, target.y - root.anchorGap)
            let topRegion = Qt.rect(source.x,
                                    source.y,
                                    source.width,
                                    (topAnchor.y < source.top) ? 0 : Math.abs(topAnchor.y - source.top))

            edges[Qt.TopEdge] = {
                anchor: topAnchor,
                region: topRegion,
                fit: topRegion.width >= window.maximumWidth
                     && topRegion.height >= window.maximumHeight
            }

            // RIGHT
            let rightAnchor = Qt.point(target.x + target.width + root.anchorGap, targetCenter.y)
            let rightRegion = Qt.rect(rightAnchor.x,
                                      source.y,
                                      (rightAnchor.x > sourceRight) ? 0 : Math.abs(sourceRight - rightAnchor.x),
                                      source.height)

            edges[Qt.RightEdge] = {
                anchor: rightAnchor,
                region: rightRegion,
                fit: rightRegion.width >= window.maximumWidth
                     && rightRegion.height >= window.maximumHeight
            }

            // BOTTOM
            let bottomAnchor = Qt.point(targetCenter.x, target.y + target.height + root.anchorGap)
            let bottomRegion = Qt.rect(source.x,
                                       bottomAnchor.y,
                                       source.width,
                                       (bottomAnchor.y > sourceBottom) ? 0 : Math.abs(sourceBottom - bottomAnchor.y))

            edges[Qt.BottomEdge] = {
                anchor: bottomAnchor,
                region: bottomRegion,
                fit: bottomRegion.width >= window.maximumWidth
                     && bottomRegion.height >= window.maximumHeight
            }

            // LEFT
            let leftAnchor = Qt.point(target.x - root.anchorGap, targetCenter.y)
            let leftRegion = Qt.rect(source.x,
                                     source.y,
                                     (leftAnchor.x < source.left) ? 0 : Math.abs(leftAnchor.x - source.left),
                                     source.height)

            edges[Qt.LeftEdge] = {
                anchor: leftAnchor,
                region: leftRegion,
                fit: leftRegion.width >= window.maximumWidth
                     && leftRegion.height >= window.maximumHeight
            }

            return edges
        }

        function popoverGeometry(edge: int, anchor: point, region: rect) {
            if (edge === Qt.TopEdge) {
                let height = Math.min(window.height, region.height)
                return Qt.rect(Math.max(region.x,
                                        Math.min(anchor.x - (window.width * 0.5),
                                                 region.x + region.width - window.width)),
                               anchor.y - height,
                               Math.min(window.width, region.width),
                               height)
            }

            if (edge === Qt.RightEdge) {
                let width = Math.min(window.width, region.width)
                return Qt.rect(anchor.x,
                               Math.max(region.y,
                                        Math.min(anchor.y - (window.height * 0.5),
                                                 region.y + region.height - window.height)),
                               width,
                               Math.min(window.height, region.height))
            }

            if (edge === Qt.BottomEdge) {
                let height = Math.min(window.height, region.height)
                return Qt.rect(Math.max(region.x,
                                        Math.min(anchor.x - (window.width * 0.5),
                                                 region.x + region.width - window.width)),
                               anchor.y,
                               Math.min(window.width, region.width),
                               height)
            }

            if (edge === Qt.LeftEdge) {
                let width = Math.min(window.width, region.width)
                return Qt.rect(anchor.x - width,
                               Math.max(region.y,
                                        Math.min(anchor.y - (window.height * 0.5),
                                                 region.y + region.height - window.height)),
                               width,
                               Math.min(window.height, region.height))
            }
        }

        onHeightChanged: {
            if (window.visible)
                root.layout()
        }

        Component.onCompleted: {
            if (window.visible)
                root.layout()
        }

        Connections {
            target: WindowManager
            enabled: root.visible

            function onFocusWindowChanged(focusWindow) {
                if (!focusWindow)
                    return

                if (root.keepOpen)
                    return

                if (focusWindow !== window && focusWindow.transientParent !== window)
                    root.close()
            }

            function onAboutToQuit() {
                root.close()
            }

            function onMainWindowVisibleChanged(value) {
                if (!value)
                    root.close()
            }
        }

        Rectangle {
            id: background
            anchors.fill: parent
            anchors.margins: window.margin
            color: StudioTheme.Values.themePopoutBackground
            border.color: "#636363"
            border.width: StudioTheme.Values.border

            TapHandler {
                id: tapHandler
                onTapped: root.close()
            }

            containmentMask: QtObject {
                function contains(point: point): bool {
                    return point.x < 0 || point.x > background.width
                        || point.y < 0 || point.y > background.height
                }
            }
        }
/*
        // The chevron will be reactivated when we fixed all the issues that where found during testing.
        // * Potential Qt bug: black background instead of transparent border due to GPU selection on Windows
        // * Ghost chevron on macOS after dragging the window
        Shape {
            id: chevron

            property int chevronWidth: window.margin

            anchors.fill: parent

            function layout(edge: int, rect: rect, anchor: point) {
                let center = Qt.point(rect.x + (rect.width * 0.5),
                                      rect.y + (rect.height * 0.5))

                // Horizontal
                if (edge === Qt.LeftEdge || edge === Qt.RightEdge) {
                    let topLimit = window.margin
                    let bottomLimit = window.height - window.margin - background.border.width
                    let point = Math.round((window.height * 0.5) + (anchor.y - center.y))

                    peak.y = Math.max(topLimit, Math.min(bottomLimit, point))

                    let topLimitChevron = topLimit + chevron.chevronWidth
                    let bottomLimitChevron = bottomLimit - chevron.chevronWidth

                    path.startY = Math.max(topLimit, Math.min(bottomLimitChevron, point - chevron.chevronWidth))
                    end.y = Math.max(topLimitChevron, Math.min(bottomLimit, point + chevron.chevronWidth))
                }

                if (edge === Qt.LeftEdge) {
                    peak.x = window.width - background.border.width
                    path.startX = end.x = window.width - window.margin - background.border.width
                }

                if (edge === Qt.RightEdge) {
                    peak.x = background.border.width
                    path.startX = end.x = window.margin + background.border.width
                }

                // Vertical
                if (edge === Qt.TopEdge || edge === Qt.BottomEdge) {
                    let leftLimit = window.margin + background.border.width
                    let rightLimit = window.width - window.margin
                    let point = Math.round((window.width * 0.5) + (anchor.x - center.x))

                    peak.x = Math.max(leftLimit, Math.min(rightLimit, point))

                    let leftLimitChevron = leftLimit + chevron.chevronWidth
                    let rightLimitChevron = rightLimit - chevron.chevronWidth

                    path.startX = Math.max(leftLimit, Math.min(rightLimitChevron, point - chevron.chevronWidth))
                    end.x = Math.max(leftLimitChevron, Math.min(rightLimit, point + chevron.chevronWidth))
                }

                if (edge === Qt.TopEdge) {
                    peak.y = window.height - background.border.width
                    path.startY = end.y = window.height - window.margin - background.border.width
                }

                if (edge === Qt.BottomEdge) {
                    peak.y = background.border.width
                    path.startY = end.y = window.margin + background.border.width
                }
            }

            ShapePath {
                id: path
                strokeStyle: ShapePath.SolidLine
                strokeWidth: background.border.width
                strokeColor: background.border.color
                fillColor: background.color
                startX: 0
                startY: 0

                PathLine { id: peak; x: 0; y: 0 }

                PathLine { id: end; x: 0; y: 0 }
            }
        }
*/
        Column {
            id: column
            anchors.fill: parent
            anchors.margins: window.margin + StudioTheme.Values.border

            Item {
                id: titleBarItem
                width: parent.width
                height: StudioTheme.Values.titleBarHeight

                DragHandler {
                    id: dragHandler

                    target: null
                    grabPermissions: PointerHandler.CanTakeOverFromAnything
                    onActiveChanged: {
                        if (dragHandler.active)
                            window.startSystemMove() // QTBUG-102488
                    }
                }

                Row {
                    id: row
                    anchors.fill: parent
                    anchors.leftMargin: StudioTheme.Values.popupMargin
                    anchors.rightMargin: StudioTheme.Values.popupMargin
                    spacing: 0

                    Item {
                        id: titleBarContent
                        width: row.width - closeIndicator.width
                        height: row.height
                    }

                    IconIndicator {
                        id: closeIndicator
                        anchors.verticalCenter: parent.verticalCenter
                        icon: StudioTheme.Constants.colorPopupClose
                        pixelSize: StudioTheme.Values.myIconFontSize
                        onClicked: root.close()
                    }
                }
            }

            Rectangle {
                width: parent.width - 8
                height: StudioTheme.Values.border
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#636363"
            }

            Basic.ScrollView {
                id: scrollView
                width: parent.width
                height: {
                    let actualHeight = mainContent.childrenRect.height + 2 * StudioTheme.Values.popupMargin

                    if (root.maximumHeight < 0)
                        return actualHeight

                    return Math.min(actualHeight,
                                    root.maximumHeight - titleBarItem.height - 3 * StudioTheme.Values.border)
                }
                padding: StudioTheme.Values.popupMargin
                clip: true

                ScrollBar.vertical: TransientScrollBar {
                    id: verticalBar
                    style: StudioTheme.Values.controlStyle
                    parent: scrollView
                    x: scrollView.width - verticalBar.width
                    y: scrollView.topPadding
                    height: scrollView.availableHeight
                    orientation: Qt.Vertical

                    show: (scrollView.hovered || scrollView.focus) && verticalBar.isNeeded
                    //otherInUse: horizontalBar.inUse
                }

                Flickable {
                    id: frame
                    boundsMovement: Flickable.StopAtBounds
                    boundsBehavior: Flickable.StopAtBounds
                    contentWidth: mainContent.width
                    contentHeight: mainContent.height

                    Item {
                        id: mainContent
                        width: scrollView.width - 2 * scrollView.padding
                        height: mainContent.childrenRect.height
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
        }
    }
}
