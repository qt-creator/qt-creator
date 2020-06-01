/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.0
import QtQuick3D 1.15

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

    property alias iconSource: iconImage.source

    signal clicked(Node node, bool multi)

    onSelectedChanged: {
        if (selected)
            hasMouse = false;
    }

    visible: activeScene === scene && (targetNode ? targetNode.visible : false)

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
            border.width: iconGizmo.highlightOnHover && iconGizmo.hasMouse ? 2 : 0
            radius: 5
            opacity: iconGizmo.selected ? 0.2 : 1
            Image {
                id: iconImage
                fillMode: Image.Pad
                MouseArea {
                    id: iconMouseArea
                    anchors.fill: parent
                    onPressed: {
                        // Ignore singleselection mouse presses when we have single object selected
                        // so that the icon gizmo doesn't hijack mouse clicks meant for other gizmos
                        if (iconGizmo.selected && !(mouse.modifiers & Qt.ControlModifier)
                                && selectedNodes.length === 1) {
                            mouse.accepted = false;
                        }
                    }

                    onClicked: iconGizmo.clicked(iconGizmo.targetNode,
                                                 mouse.modifiers & Qt.ControlModifier)
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
