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
    id: root
    property Node targetNode
    property View3D targetView

    property vector3d offset: Qt.vector3d(0, 0, 0)

    property bool isBehindCamera

    onTargetNodeChanged: updateOverlay()

    Connections {
        target: targetNode
        function onSceneTransformChanged() { updateOverlay() }
    }

    Connections {
        target: targetView.camera
        function onSceneTransformChanged() { updateOverlay() }
    }

    Connections {
        target: _generalHelper
        function onOverlayUpdateNeeded() { updateOverlay() }
    }

    function updateOverlay()
    {
        var scenePos = targetNode ? targetNode.scenePosition : Qt.vector3d(0, 0, 0);
        // Need separate variable as scenePos is reference to read-only property
        var scenePosWithOffset = Qt.vector3d(scenePos.x + offset.x,
                                             scenePos.y + offset.y,
                                             scenePos.z + offset.z);
        var viewPos = targetView ? targetView.mapFrom3DScene(scenePosWithOffset)
                                 : Qt.vector3d(0, 0, 0);
        root.x = viewPos.x;
        root.y = viewPos.y;

        isBehindCamera = viewPos.z <= 0;
    }
}
