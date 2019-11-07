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

Node {
    id: iconGizmo

    property View3D view3D
    property bool highlightOnHover: true
    property Node targetNode: null

    property alias gizmoModel: gizmoModel
    property alias iconSource: iconImage.source

    signal positionCommit()
    signal selected(Node node)

    position: targetNode ? targetNode.scenePosition : Qt.vector3d(0, 0, 0)
    rotation: targetNode ? targetNode.sceneRotation : Qt.vector3d(0, 0, 0)
    visible: targetNode ? targetNode.visible : false

    Model {
        id: gizmoModel
        visible: iconGizmo.visible
    }
    Overlay2D {
        id: gizmoLabel
        targetNode: gizmoModel
        targetView: view3D
        offsetX: 0
        offsetY: 0
        visible: iconGizmo.visible && !isBehindCamera
        parent: view3D

        Rectangle {
            width: 24
            height: 24
            x: -width / 2
            y: -height
            color: "transparent"
            border.color: "#7777ff"
            border.width: highlightOnHover && iconMouseArea.containsMouse ? 2 : 0
            radius: 5
            Image {
                id: iconImage
                anchors.fill: parent
                MouseArea {
                    id: iconMouseArea
                    anchors.fill: parent
                    onClicked: selected(targetNode)
                    hoverEnabled: highlightOnHover
                }
            }
        }
    }
}
