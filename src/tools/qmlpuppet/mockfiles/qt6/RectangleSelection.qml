// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Shapes
import QtQuick.Studio.Components

RectangleItem {
    x: 0
    y: 0
    width: 0
    height: 0
    radius: 0
    strokeWidth: 2
    fillColor: "#20000000"
    strokeColor: "#FFFFFFFF"
    strokeStyle: ShapePath.DashLine

    enum State {
        Stopped,
        Started,
        Showing
    }

    property Item view3D: null
    property point startPoint: Qt.point(-1, -1)
    property int state: RectangleSelection.State.Stopped
    property bool supported: _generalHelper.isPickInRectSupported

    function start(x, y) {
        startPoint.x = x
        startPoint.y = y
        state = RectangleSelection.State.Started
    }

    function stop() {
        width = 0
        height = 0
        strokeWidth = 0
        state = RectangleSelection.State.Stopped
    }

    function show(endX, endY) {
        strokeWidth = 2
        x = Math.min(endX, startPoint.x)
        y = Math.min(endY, startPoint.y)
        width = Math.abs(startPoint.x - endX)
        height = Math.abs(startPoint.y - endY)
        state = RectangleSelection.State.Showing
    }

    function clamp(val, min, max) {
        return Math.max(min, Math.min(max, val))
    }

    function pickInRect(mouse) {
        var index = view3D.activeViewport
        var rect = view3D.viewRects[index]
        var mouseX = clamp(mouse.x, rect.x + strokeWidth, rect.x + (rect.width - strokeWidth))
        var mouseY = clamp(mouse.y, rect.y + strokeWidth, rect.y + (rect.height - strokeWidth))
        show(mouseX, mouseY)
        var view3d = view3D.editViews[index]
        var ret = _generalHelper.pickInRect(view3d, startPoint, Qt.point(mouseX, mouseY))
        checkGizmos(startPoint, Qt.point(mouseX, mouseY), ret)
        return ret
    }

    function isInside(point, min, max) {
        return ((point.x >= min.x && point.x <= max.x) && (point.y >= min.y && point.y <= max.y))
    }

    function checkGizmoIcon(gizmo, start, end) {
        var iconMinX = gizmo.iconOverlay.x - gizmo.iconImage.width * 0.5
        var iconMinY = gizmo.iconOverlay.y - gizmo.iconImage.height * 0.5
        var iconMaxX = gizmo.iconOverlay.x + gizmo.iconImage.width * 0.5
        var iconMaxY = gizmo.iconOverlay.y + gizmo.iconImage.height * 0.5

        var point1 = Qt.point(iconMinX, iconMinY)
        var point2 = Qt.point(iconMinX, iconMaxY)
        var point3 = Qt.point(iconMaxX, iconMaxY)
        var point4 = Qt.point(iconMaxX, iconMinY)

        var rectMin = Qt.point(Math.min(start.x, end.x), Math.min(start.y, end.y))
        var rectMax = Qt.point(Math.max(start.x, end.x), Math.max(start.y, end.y))

        return (isInside(point1, rectMin, rectMax)
                || isInside(point2, rectMin, rectMax)
                || isInside(point3, rectMin, rectMax)
                || isInside(point4, rectMin, rectMax))
    }

    function checkIconGizmos(iconGizmos, start, end, dst) {
        for (var i = 0; i < iconGizmos.length; ++i) {
            var gizmo = iconGizmos[i]
            if (gizmo.targetNode && checkGizmoIcon(gizmo, start, end))
                dst[dst.length] = gizmo.targetNode
        }
    }

    function checkGizmos(start, end, dst) {
        var overlayView = view3D.activeOverlayView
        checkIconGizmos(overlayView.lightIconGizmos, start, end, dst)
        checkIconGizmos(overlayView.cameraGizmos, start, end, dst)
        checkIconGizmos(overlayView.particleSystemIconGizmos, start, end, dst)
    }
}
