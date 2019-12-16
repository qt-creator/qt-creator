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
import QtQuick3D 1.0
import QtGraphicalEffects 1.12

Node {
    id: iconGizmo

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

    property alias iconSource: iconImage.source
    property alias overlayColor: colorOverlay.color

    signal positionCommit()
    signal clicked(Node node, bool multi)

    position: targetNode ? targetNode.scenePosition : Qt.vector3d(0, 0, 0)
    rotation: targetNode ? targetNode.sceneRotation : Qt.vector3d(0, 0, 0)
    visible: targetNode ? targetNode.visible : false

    Overlay2D {
        id: iconOverlay
        targetNode: iconGizmo
        targetView: view3D
        visible: iconGizmo.visible && !isBehindCamera
        parent: view3D

        Rectangle {
            id: iconRect
            width: iconImage.width
            height: iconImage.height
            x: -width / 2
            y: -height / 2
            color: "transparent"
            border.color: "#7777ff"
            border.width: !iconGizmo.selected
                          && iconGizmo.highlightOnHover && iconMouseArea.containsMouse ? 2 : 0
            radius: 5
            opacity: iconGizmo.selected ? 0.2 : 1
            Image {
                id: iconImage
                fillMode: Image.Pad
                MouseArea {
                    id: iconMouseArea
                    anchors.fill: parent
                    onPressed: {
                        if (iconGizmo.selected && !(mouse.modifiers & Qt.ControlModifier)) {
                            mouse.accepted = false;
                        }
                    }

                    onClicked: iconGizmo.clicked(iconGizmo.targetNode,
                                                 mouse.modifiers & Qt.ControlModifier)
                    hoverEnabled: iconGizmo.highlightOnHover && !iconGizmo.selected
                    acceptedButtons: Qt.LeftButton
                }
            }
            ColorOverlay {
                id: colorOverlay
                anchors.fill: parent
                cached: true
                source: iconImage
                color: "transparent"
                opacity: 0.6
            }

        }
    }
}
