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
import MouseArea3D 1.0

Node {
    id: overlayNode

    property View3D view3D
    property Camera camera: view3D.camera
    property bool active: true

    // Read-only
    property real relativeScale: 1

    onActiveChanged: updateScale()
    onSceneTransformChanged: updateScale()
    // Trigger delayed update on camera change to ensure camera values are correct
    onCameraChanged: _generalHelper.requestOverlayUpdate();

    Connections {
        target: camera
        function onSceneTransformChanged() { updateScale() }
    }

    Connections {
        target: _generalHelper
        function onOverlayUpdateNeeded() { updateScale() }
    }

    function getScale(baseScale)
    {
        return Qt.vector3d(baseScale.x * relativeScale, baseScale.y * relativeScale,
                           baseScale.z * relativeScale);
    }

    function updateScale()
    {
        if (active)
            relativeScale = helper.getRelativeScale(overlayNode);
        else
            relativeScale = 1;
    }

    MouseArea3D {
        id: helper
        active: false
        view3D: overlayNode.view3D
    }
}
