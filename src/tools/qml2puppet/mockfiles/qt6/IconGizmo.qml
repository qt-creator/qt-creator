// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0

Item {
    id: iconGizmo

    property Node activeScene: null
    property Node scene: null
    property View3D view3D
    property bool highlightOnHover: true
    property Node targetNode: null
    property var selectedNodes: []
    readonly property bool selected: {
        for (var i = 0; i < selectedNodes.length; ++i) {
            if (selectedNodes[i] === targetNode)
                return true;
        }
        return false;
    }
    property bool hasMouse: false
    property bool hidden: false
    property bool locked: false
    property bool globalShow: true
    property bool canBeVisible: activeScene === scene && !hidden && (targetNode ? targetNode.visible : false)
    property real iconOpacity: selected ? 0.2 : 1

    property alias iconSource: iconImage.source

    signal clicked(Node node, bool multi)

    function hasPoint(x, y)
    {
        if (!view3D || !targetNode)
            return false;

        var point =  view3D.mapToItem(iconMouseArea, x, y);

        return point.x >= iconMouseArea.x && (point.x <= iconMouseArea.x + iconMouseArea.width)
                && point.y >= iconMouseArea.y && (point.y <= iconMouseArea.y + iconMouseArea.height);
    }

    onSelectedChanged: {
        if (selected)
            hasMouse = false;
    }

    visible: canBeVisible && globalShow

    Overlay2D {
        id: iconOverlay
        targetNode: iconGizmo.targetNode
        targetView: view3D
        visible: iconGizmo.visible && !isBehindCamera

        Rectangle {
            id: iconRect

            width: iconImage.width
            height: iconImage.height
            x: -width / 2
            y: -height / 2
            color: "transparent"
            border.color: "#7777ff"
            border.width: !iconGizmo.locked && iconGizmo.highlightOnHover && iconGizmo.hasMouse ? 2 : 0
            radius: 5
            opacity: iconGizmo.iconOpacity
            Image {
                id: iconImage
                fillMode: Image.Pad
                MouseArea {
                    id: iconMouseArea
                    anchors.fill: parent
                    onPressed: (mouse)=> {
                        // Ignore singleselection mouse presses when we have single object selected
                        // so that the icon gizmo doesn't hijack mouse clicks meant for other gizmos
                        if (iconGizmo.selected && !(mouse.modifiers & Qt.ControlModifier)
                                && selectedNodes.length === 1) {
                            mouse.accepted = false;
                        }
                    }

                    onClicked: (mouse)=> {
                        iconGizmo.clicked(iconGizmo.targetNode,
                                          mouse.modifiers & Qt.ControlModifier);
                    }
                    hoverEnabled: iconGizmo.highlightOnHover && !iconGizmo.selected
                    acceptedButtons: Qt.LeftButton

                    // onPositionChanged, onContainsMouseAreaChanged, and hasMouse are used instead
                    // of just using containsMouse directly, because containsMouse
                    // cannot be relied upon to update correctly in some situations.
                    // This is likely because the overlapping 3D mouse areas of the gizmos get
                    // the mouse events instead of this area, so mouse leaving the area
                    // doesn't always update containsMouse property.
                    onPositionChanged: {
                        if (!iconGizmo.selected)
                            iconGizmo.hasMouse = containsMouse;
                    }

                    onContainsMouseChanged: {
                        if (!iconGizmo.selected)
                            iconGizmo.hasMouse = containsMouse;
                        else
                            iconGizmo.hasMouse = false;
                    }
                }
            }
        }
    }
}
